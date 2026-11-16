#include "kernel/disc.h"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

void DumpDir(const kernel::Disc& disc, const kernel::Disc::Entry& dir, const std::string& prefix = "") {
    for (const auto& child : dir.children) {
        std::cout << prefix << (child.dir ? "[DIR]  " : "[FILE] ") << child.name 
                  << " (size: " << child.size << ")" << std::endl;
        if (child.dir) {
            DumpDir(disc, child, prefix + "  ");
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: extract_xbe <iso_path> [out_dir]\n";
        return 1;
    }

    fs::path iso_path = argv[1];
    fs::path out_dir = (argc > 2) ? argv[2] : iso_path.parent_path();

    kernel::Disc disc;
    std::string err;
    if (!disc.Open(iso_path, err)) {
        std::cerr << "Failed to open ISO: " << err << "\n";
        return 1;
    }
    std::cout << "Successfully opened Xbox ISO: " << iso_path << "\n";

    // Extract default.xbe
    std::vector<std::string> xbes = {"default.xbe", "sc4_adv.xbe"};
    for (const auto& name : xbes) {
        std::vector<uint8_t> data;
        if (disc.Load(name, data)) {
            fs::path out_path = out_dir / name;
            std::ofstream out(out_path, std::ios::binary);
            out.write(reinterpret_cast<const char*>(data.data()), data.size());
            std::cout << "Extracted: " << name << " -> " << out_path << " (" << data.size() << " bytes)\n";
        } else {
            std::cerr << "Could not find " << name << " in ISO!\n";
        }
    }

    return 0;
}
