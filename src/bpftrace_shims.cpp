// Definitions bpftrace's AST code needs from files we deliberately do not
// compile, because they pull in the whole bpftrace runtime (bcc, libbpf
// loading, ...).
//
// Keep each function identical to its upstream counterpart, except stubs
// for libraries that are not linked at all.

#include "attached_probe.h"
#include "log.h"

namespace bpftrace {

// Copied from bpftrace/src/attached_probe.cpp.
bpf_prog_type progtype(ProbeType t)
{
    // clang-format off
    switch (t) {
        case ProbeType::special:    return BPF_PROG_TYPE_RAW_TRACEPOINT; break;
        case ProbeType::test:       return BPF_PROG_TYPE_XDP; break;
        case ProbeType::benchmark:  return BPF_PROG_TYPE_XDP; break;
        case ProbeType::kprobe:     return BPF_PROG_TYPE_KPROBE; break;
        case ProbeType::kretprobe:  return BPF_PROG_TYPE_KPROBE; break;
        case ProbeType::uprobe:     return BPF_PROG_TYPE_KPROBE; break;
        case ProbeType::uretprobe:  return BPF_PROG_TYPE_KPROBE; break;
        case ProbeType::usdt:       return BPF_PROG_TYPE_KPROBE; break;
        case ProbeType::tracepoint: return BPF_PROG_TYPE_TRACEPOINT; break;
        case ProbeType::profile:    return BPF_PROG_TYPE_PERF_EVENT; break;
        case ProbeType::interval:   return BPF_PROG_TYPE_PERF_EVENT; break;
        case ProbeType::software:   return BPF_PROG_TYPE_PERF_EVENT; break;
        case ProbeType::watchpoint: return BPF_PROG_TYPE_PERF_EVENT; break;
        case ProbeType::hardware:   return BPF_PROG_TYPE_PERF_EVENT; break;
        case ProbeType::fentry:     return BPF_PROG_TYPE_TRACING; break;
        case ProbeType::fexit:      return BPF_PROG_TYPE_TRACING; break;
        case ProbeType::iter:       return BPF_PROG_TYPE_TRACING; break;
        case ProbeType::rawtracepoint: return BPF_PROG_TYPE_TRACING; break;
        case ProbeType::invalid:
            LOG(BUG) << "program type invalid";
    }
    // clang-format on

    return {}; // unreached
}

} // namespace bpftrace

// libbpf is not linked. ast.cpp's shared_ptr release code can reach
// btf::detail::Handle's destructor, which calls btf__free, but bpfls never
// loads BTF, so no Handle (and no struct btf) is ever created.
extern "C" void btf__free(struct btf *)
{
}
