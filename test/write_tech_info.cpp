#include <cbag/cbag.h>
#include <cbagoa/cbagoa.h>

void write_tech_info_file(const char *fname, const char *tech_lib, const char *lib_file = nullptr) {
    cbag::init_logging();

    std::string lib_str("cds.lib");
    if (lib_file != nullptr) {
        lib_str = lib_file;
    }

    cbagoa::oa_database db(lib_str);
    db.write_tech_info_file(fname, tech_lib);
}

int main(int argc, char *argv[]) {
    if (argc == 3) {
        write_tech_info_file(argv[1], argv[2]);
    } else if (argc == 4) {
        write_tech_info_file(argv[1], argv[2], argv[3]);
    } else {
        std::cout << "Usage: write_tech_info <fname> <tech_lib> [<cds_lib_fname>]" << std::endl;
    }
    return 0;
}
