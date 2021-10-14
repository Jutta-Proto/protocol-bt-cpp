#pragma once

#include <memory>
#include <optional>
#include <string>

//---------------------------------------------------------------------------
namespace bt {
//---------------------------------------------------------------------------
struct ScanArgs {
    std::mutex doneMutex;
    std::mutex m;
    std::string name;
    bool success{false};
    std::string addr;
} __attribute__((aligned(128)));

std::shared_ptr<ScanArgs> scan_for_device(std::string&& name);
//---------------------------------------------------------------------------
}  // namespace bt
//---------------------------------------------------------------------------