//
// Created by corne on 10/30/2021.
//

#include "args.hpp"

#include <iostream>
#include <fstream>
#include <iomanip>

//! Read input string as any base
struct AnyBaseReader {
    void operator()(const std::string &name, const std::string &value, int &destination) {
        // autodetect base
        auto converted = std::stoi(value, nullptr, 0);
        destination = converted;
    }
};

//! Execute XOR mask @ingroup cli
void Xor(args::Subparser &parser);

//! Execute Hex mask @ingroup cli
void Hex(args::Subparser &parser);

//! Execute Binary mask @ingroup cli
void Bin(args::Subparser &parser);

args::Group arguments("arguments");
args::Flag verbose(arguments, "verbose", "Show verbose output", {"verbose"});

int main(int argc, char **argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);

    std::vector<const char*> wrappedArgs(argv, argv+argc);
    auto hasStdin = std::cin.rdbuf()->in_avail();
    printf("%ld\n", hasStdin);
    if(hasStdin > 0) {
        std::string piped;
        std::cin >> piped;
        wrappedArgs.emplace_back(piped.c_str());
    }

    for(const auto&a:wrappedArgs){
        printf("arg: %s\n", a);
    }

    args::ArgumentParser p("mask");
    args::HelpFlag help(p, "help", "Display this help menu", {'h', "help"});
    args::Flag version(p, "version", "Show the version of this program", {'v', "version"});
    args::CompletionFlag complete(p, {"complete"});
    p.Prog(argv[0]);
    p.ProglinePostfix("{command options}");

    args::GlobalOptions globals(p, arguments);
    args::Group commands(p, "commands");
    args::Command _xor(commands, "xor", "Byte-wise XOR input with a specified key", &Xor);
    args::Command _hex(commands, "hex", "Byte-wise hex encoding of an input", &Hex);
    args::Command _bin(commands, "bin", "Byte-wise binary encoding of an input", &Bin);
    _xor.KickOut(true);

    try {
        auto next = p.ParseArgs(args);
        std::cout << std::boolalpha;
        if (bool{version}) {
            std::cout << "mask v"
                      << MASK_VERSION_MAJOR << "."
                      << MASK_VERSION_MINOR << "."
                      << MASK_VERSION_PATCH << "."
                      << MASK_VERSION_TWEAK << " "
                      << "Copyright @ 2021 Cory Todd "
                      << std::endl;
        }
    }
    catch (args::Help &) {
        std::cout << p;
        return 0;
    } catch (args::Completion &e) {
        std::cerr << e.what();
    }
    catch (args::Error &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << p;
        return 1;
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}

void Xor(args::Subparser &parser) {

    // Requires parameters: <input message> <key>
    args::Positional<std::string> argMessage(parser, "message", "input message to XOR", args::Options::Required);
    args::Positional<int, AnyBaseReader> argKey(parser, "key",
                                                "XOR key. Use 0x prefix for hex, otherwise base 10 is assumed",
                                                args::Options::Required);
    parser.Parse();

    // Capture parameters
    auto input = args::get(argMessage);
    auto key = args::get(argKey);
    std::stringstream ss;

    if (bool{verbose}) {
        std::cout << "mask::xor " << input << " ^ " << key << " = ";
    }

    // XOR each input character
    for (const auto &ch: input) {
        auto xCh = ch ^ key;
        ss << "\\x" << std::hex << std::uppercase << std::setw(2) << xCh;
    }

    std::cout << ss.str() << std::endl;
}

void Hex(args::Subparser &parser) {

    // Requires parameters: <input message> <key>
    args::Positional<std::string> argMessage(parser, "message", "input message to XOR", args::Options::Required);
    parser.Parse();

    // Capture parameters
    auto input = args::get(argMessage);
    std::stringstream ss;

    if (bool{verbose}) {
        std::cout << "mask::hex " << input << " = ";
    }

    // XOR each input character
    for (const auto &ch: input) {
        ss << "\\x" << std::hex << std::uppercase << std::setw(2) << (int) ch;
    }

    std::cout << ss.str() << std::endl;
}

void Bin(args::Subparser &parser) {

    // Requires parameters: <input string>
    args::ValueFlag<std::string> argInFile(parser, "input", "input file path", {'i'}, args::Options::Required);
    args::ValueFlag<std::string> argOutFile(parser, "output", "output file path", {'o'}, args::Options::Required);
    parser.Parse();

    // Capture parameters
    auto inputFile = args::get(argInFile);
    std::stringstream ss;

    if (bool{verbose}) {
        std::cout << "mask::bin " << inputFile.c_str();
    }

    bool hasZero = false;
    bool hasEx = false;
    char ch;

    std::vector<uint8_t> binary;
    binary.reserve(4096);

    // Read and parser input from file: 0xNN where NN is ASCII 0-9, a-f, or A-F
    std::ifstream reader(inputFile.c_str(), std::ios::in);
    while (reader.is_open() && reader.get(ch)) {
        switch (ch) {
            case '0':
                if (!hasZero) {
                    hasZero = true;
                } else {
                    ss << ch;
                }
                break;
            case 'x':
            case 'X':
                hasEx = true;
                break;
            case ',':
            case '\n':
                hasZero = false;
                hasEx = false;
                binary.emplace_back(std::stoi(ss.str(), nullptr, 16));
                ss.str(std::string());
                break;
            case ' ':
            case '\t':
                // Ignore
                break;
            default:
                if ((ch >= 'a' && ch <= 'z' ||
                     ch >= 'A' && ch <= 'Z' ||
                     ch >= '0' && ch <= '9') &&
                    hasZero && hasEx) {
                    ss << ch;
                }
                break;
        }
    }

    // Handle any remaining data
    if (!ss.str().empty()) {
        binary.emplace_back(std::stoi(ss.str(), nullptr, 16));
        ss.clear();
    }

    // Clean up reader
    if (!reader.eof() && reader.fail()) {
        throw std::runtime_error("Failed to open input file");
    }
    reader.close();

    // Write result
    std::ofstream writer(args::get(argOutFile), std::ios::out | std::ios::binary);
    if (writer.is_open()) {
        for (const auto &b: binary) {
            writer.write((char *) &b, sizeof(b));
        }
    }

    // Clean up writer
    if (!writer.eof() && writer.fail()) {
        throw std::runtime_error("Failed to write to output file");
    }
    writer.close();
}
