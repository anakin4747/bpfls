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
});

CPPSPEC_MAIN(document_test);
