// $Id: e57unpack.cpp 246 2011-10-06 08:01:00Z roland_schwarz $
#define PROGRAM_NAME "e57unpack"
#define PROGRAM_VERSION "0.1"
#ifndef SVN_VERSION
#define SVN_VERSION unknown
#endif
#define STRING_HELPER2(a) #a
#define STRING_HELPER1(a) STRING_HELPER2(a)
#define BUILD_VERSION STRING_HELPER1(SVN_VERSION)


#include "E57Foundation.h"
using e57::Node;
using e57::ImageFile;
using e57::StructureNode;
using e57::VectorNode;
using e57::CompressedVectorNode;
using e57::StringNode;
using e57::IntegerNode;
using e57::ScaledIntegerNode;
using e57::FloatNode;
using e57::StringNode;
using e57::BlobNode;
using e57::E57Exception;
using e57::E57Utilities;
using e57::ustring;
using e57::SourceDestBuffer;
using e57::CompressedVectorReader;
//using e57::int64_t;
//using e57::uint64_t;

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;

#include <exception>
using std::exception;

#include <stdexcept>
using std::runtime_error;

#if defined(_MSC_VER) || defined(WASM)
#   include <memory>
using std::shared_ptr;
#else
#   include <tr1/memory>
using std::tr1::shared_ptr;
#endif

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <boost/config.hpp>

#include <boost/program_options.hpp>
using boost::program_options::variables_map;
using boost::program_options::options_description;
using boost::program_options::positional_options_description;
using boost::program_options::store;
using boost::program_options::command_line_parser;
using boost::program_options::value;

#include <boost/filesystem.hpp>
using boost::filesystem::path;
using boost::filesystem::create_directories;
using boost::filesystem::portable_directory_name;

#include <boost/filesystem/fstream.hpp>
using boost::filesystem::ofstream;

#include <boost/variant.hpp>
using boost::variant;
using boost::get;
using boost::static_visitor;
using boost::apply_visitor;

#include <boost/format.hpp>
using boost::format;
using boost::io::too_many_args_bit;
using boost::io::all_error_bits;

variables_map opt;

void
print_version(
) {
    cout
    << PROGRAM_NAME" (" << BOOST_PLATFORM << ") "
    << PROGRAM_VERSION
    << "." << BUILD_VERSION
    << endl
    ;

    int astmMajor;
    int astmMinor;
    ustring libraryId;
    E57Utilities().getVersions(astmMajor, astmMinor, libraryId);

    cout << "E57 API " << astmMajor << "." << astmMinor << endl;
    cout << libraryId << endl;
}

void
print_help(
    const options_description& options
) {
    cout
    << "Usage:\n"
    << PROGRAM_NAME" [options] e57-file\n"
    << "  The purpose of this program is to break the e57 file into\n"
    << "  parts.\n"
    << endl
    << options
    ;
}

class get_at
    : public static_visitor<variant<double, int64_t> >
{
    size_t at;
public:
    get_at(size_t at_) : at(at_) {}
    template <typename T>
    variant<double, int64_t> operator()( T & operand ) const
    {
        return operand[at];
    }

};

path
path_from_guid(
    string guid
) {
    string name;
    for (string::iterator s = guid.begin(); s != guid.end(); ++s) {
        if (portable_directory_name(string(1,*s)))
            name += *s;
        else
            name += "_";
    }
    return path(name);
}

int
main(
    int argc
    , char *argv[]
) {
    try {
        options_description options("options");
        options.add_options()
            (
                "version,v"
                , "show version"
            )
            (
                "help,h"
                , "show help"
            )
            (
                "format,F"
                , value<std::string>()
                ,"a boost::format format string for\n"
                 "the pointrecord data.\n"
            )
            (
                "src"
                , value<string>()
                , "define e57 file\n"
                  "  (--src may be omitted)\n"
            )
            (
                "dst"
                , value<string>()
                , "the destination directory\n"
                  "  (--dst may be omitted)"
            )
            ;

        positional_options_description  positional;
        positional.add("src",1);
        positional.add("dst",2);

        store(
            command_line_parser(argc, argv)
                . options(options)
                . positional(positional)
                . run()
            , opt
        );

        if (opt.count("version")) {
            print_version();
            return 0;
        }

        if (opt.count("help")) {
            print_help(options);
            return 0;
        }

        if (opt.count("src")) {
            string fmt;
            if (opt.count("format"))
                fmt = opt["format"].as<string>();

            ImageFile imf(opt["src"].as<string>(), "r");
            StructureNode root = imf.root();

            path dst;
            if (opt.count("dst"))
                dst = path_from_guid(opt["dst"].as<string>());
            else
                dst = path_from_guid(StringNode(root.get("guid")).value());

            create_directories(dst);

            if (root.isDefined("data3D")) {
                VectorNode data3D(root.get("data3D"));
                for (int64_t i=0; i<data3D.childCount(); ++i) {
                    StructureNode            scan(data3D.get(i));
                    CompressedVectorNode     points(scan.get("points"));
                    StructureNode            prototype(points.prototype());
                    vector<SourceDestBuffer> sdb;
                    const size_t buf_size = 1024;
                    vector<variant<vector<double>, vector<int64_t> > > buf;
                    string pointrecord;

                    string comma;
                    for (int64_t i=0; i<prototype.childCount(); ++i) {
                        switch(prototype.get(i).type()) {
                            case e57::E57_FLOAT:
                            case e57::E57_SCALED_INTEGER:
                                buf.push_back(vector<double>(buf_size));
                                if (!opt.count("format"))
                                    fmt += comma +"%g";
                                break;
                            case e57::E57_INTEGER:
                                buf.push_back(vector<int64_t>(buf_size));
                                if (!opt.count("format"))
                                    fmt += comma +"%d";
                                break;
                        }
                        if (comma.empty()) comma = ",";
                    }

                    comma.clear();
                    for (size_t i=0; i<prototype.childCount(); ++i) {
                        Node n(prototype.get(i));
                        pointrecord += comma + n.elementName();
                        if (comma.empty()) comma = ",";
                        switch(n.type()) {
                            case e57::E57_FLOAT:
                            case e57::E57_SCALED_INTEGER:
                                sdb.push_back(
                                    SourceDestBuffer(
                                        imf
                                        , n.elementName()
                                        , &get<vector<double> >(buf[i])[0]
                                        , buf_size
                                        , true
                                        , true
                                    )
                                );
                                break;
                            case e57::E57_INTEGER:
                                sdb.push_back(
                                    SourceDestBuffer(
                                        imf
                                        , n.elementName()
                                        , &get<vector<int64_t> >(buf[i])[0]
                                        , buf_size
                                        , true
                                        , true
                                    )
                                );
                            break;
                            default:
                                throw(runtime_error(
                                    "prototype contains illegal type")
                            );
                        }
                    }

                    boost::filesystem::ofstream inf(
                        dst/path_from_guid(StringNode(scan.get("guid")).value()+".inf")
                    );
                    inf << "pointrecord: " << pointrecord << endl;
                    inf << "format:      " << fmt << endl;
                    inf.close();

                    CompressedVectorReader rd(points.reader(sdb));
                    path csvname(path_from_guid(StringNode(scan.get("guid")).value()+".csv"));
                    boost::filesystem::ofstream ocsv(dst/csvname);
                    ostream& out(ocsv); // needed to fix ambiguity for << operator on msvc
                    cout << "unpacking: " << dst/csvname << " ... ";
                    unsigned count;
                    uint64_t total_count(0);

                    format tfmt(fmt);
                    tfmt.exceptions( all_error_bits ^ too_many_args_bit );
                    while(count = rd.read()) {
                        total_count += count;
                        for (size_t i=0; i<count; ++i) {
                            for (size_t j=0; j<buf.size(); ++j)
                                tfmt = tfmt % apply_visitor(get_at(i),buf.at(j));
                            out << tfmt << endl;
                        }
                    }
                    cout << " total points: " << total_count << endl;

                    ocsv.close();
                }
            }

            if (root.isDefined("cameraImages")) {
                VectorNode cameraImages(root.get("cameraImages"));
                cout << "unpacking of cameraImages not supported yet." << endl;
            }

            return 0;
        }
        else {
            print_help(options);
            return -1;
        }

    } catch(E57Exception& e) {
        e.report(__FILE__, __LINE__, __FUNCTION__);
        return -1;
    }
    catch(exception& e) {
        cerr << e.what() << endl;
        return -1;
    }
    catch(...) {
        cerr << "unknown exception" << endl;
        return -1;
    }
    return 0;
}
