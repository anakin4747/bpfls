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
});

CPPSPEC_MAIN(document_test);
