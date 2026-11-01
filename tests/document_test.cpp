#include <string>

// cppspec defines macros such as `it`, which clash with identifiers in
// bpftrace's headers, so project headers must be included first.
#include "document.h"

#include "cppspec.hpp"

describe document_test("Document", $ {
    it("stores the script contents", _ {
        std::string script = "BEGIN { print(\"hello\"); }";
        bpfls::Document doc("file:///hello.bt", script);
        expect(doc.text()).to_equal(script);
    });

    it("parses the contents with the bpftrace parser", _ {
        bpfls::Document doc("file:///hello.bt", "BEGIN { print(\"hello\"); }");
        expect(doc.ast().root != nullptr).to_be_true();
        expect(doc.ast().diagnostics().ok()).to_be_true();
    });

    context("when the script has syntax errors", _ {
        it("reports an error in every broken probe", _ {
            bpfls::Document doc("file:///broken.bt",
                                "BEGIN { $x = ; }\n"
                                "END { print( }\n");
            expect(doc.ast().diagnostics().error_count()).to_equal(2UL);
        });

        it("keeps parsing the probes after a broken one", _ {
            bpfls::Document doc("file:///broken.bt",
                                "BEGIN { $x = ; }\n"
                                "END { print(1); }\n");
            expect(doc.ast().root != nullptr).to_be_true();
            expect(doc.ast().root->probes.size()).to_equal(2UL);
        });

        it("reports one error for one mistake", _ {
            bpfls::Document doc("file:///broken.bt",
                                "BEGIN { $x = ; print(1); print(2); }\n");
            expect(doc.ast().diagnostics().error_count()).to_equal(1UL);
        });
    });

    context("node_at", _ {
        // Columns of the first line:
        // BEGIN { $x = 1 + 22; }
        // 0     6 8   12  16 19
        it("finds a variable", _ {
            bpfls::Document doc("file:///t.bt", "BEGIN { $x = 1 + 22; }\n");
            auto *var = dynamic_cast<bpftrace::ast::Variable *>(
                doc.node_at({ .line = 0, .character = 9 }));
            expect(var != nullptr).to_be_true();
            if (var != nullptr) {
                expect(var->ident).to_equal(std::string("$x"));
            }
        });

        it("returns the innermost node rather than an enclosing one", _ {
            bpfls::Document doc("file:///t.bt", "BEGIN { $x = 1 + 22; }\n");
            auto *num = dynamic_cast<bpftrace::ast::Integer *>(
                doc.node_at({ .line = 0, .character = 18 }));
            expect(num != nullptr).to_be_true();
            if (num != nullptr) {
                expect(num->value).to_equal(22UL);
            }
        });

        it("finds the attach point of a probe", _ {
            bpfls::Document doc("file:///t.bt", "BEGIN { $x = 1 + 22; }\n");
            auto *ap = dynamic_cast<bpftrace::ast::AttachPoint *>(
                doc.node_at({ .line = 0, .character = 2 }));
            expect(ap != nullptr).to_be_true();
        });

        it("returns nullptr on blank lines between probes", _ {
            bpfls::Document doc("file:///t.bt",
                                "BEGIN { $x = 1; }\n"
                                "\n"
                                "END { $y = 2; }\n");
            expect(doc.node_at({ .line = 1, .character = 0 }) == nullptr)
                .to_be_true();
        });

        it("finds nodes on later lines", _ {
            bpfls::Document doc("file:///t.bt",
                                "BEGIN { $x = 1; }\n"
                                "\n"
                                "END { $y = 2; }\n");
            auto *var = dynamic_cast<bpftrace::ast::Variable *>(
                doc.node_at({ .line = 2, .character = 7 }));
            expect(var != nullptr).to_be_true();
            if (var != nullptr) {
                expect(var->ident).to_equal(std::string("$y"));
            }
        });

        it("finds nodes in a valid probe after a broken one", _ {
            bpfls::Document doc("file:///broken.bt",
                                "BEGIN { $x = ; }\n"
                                "END { $y = 2; }\n");
            auto *var = dynamic_cast<bpftrace::ast::Variable *>(
                doc.node_at({ .line = 1, .character = 7 }));
            expect(var != nullptr).to_be_true();
            if (var != nullptr) {
                expect(var->ident).to_equal(std::string("$y"));
            }
        });
    });
});

CPPSPEC_MAIN(document_test);
