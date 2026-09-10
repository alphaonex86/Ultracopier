/* Standalone unit test for extractProtocol() (sources/cpp11addition.cpp).
 *
 * extractProtocol() is what decides, for every source and destination Ultracopier is handed,
 * whether the string is a LOCAL path or carries a protocol nobody may be able to handle. Core
 * refuses a transfer whose protocols no copy engine supports, so a wrong answer here is either a
 * lost transfer (a local path misread as a protocol) or the bug this whole change is about: a
 * remote URL treated as "file", silently doing nothing on the source side and creating a junk
 * LOCAL directory named after the URL on the destination side.
 *
 * The classifier also has to survive strings that are not paths at all, so the hostile shapes
 * (a lone ':', ':/' at position 0, a name that is only letters, an embedded NUL) are checked too.
 * Run by cases/protocol_unit.py -- exit code 0 means every check passed. */
#include <iostream>
#include <string>

#include "cpp11addition.h"

static unsigned int failures = 0;

static void check(const std::string &input, const std::string &expected)
{
    const std::string got = extractProtocol(input);
    if (got != expected) {
        ++failures;
        std::cout << "FAIL: extractProtocol(\"" << input << "\") = \"" << got
                  << "\", expected \"" << expected << "\"" << std::endl;
    }
}

int main()
{
    // --- local paths: everything that is not a scheme must answer "file" -------------------
    check("/home/user/file.txt", "file");
    check("/home/user/dir/", "file");
    check("relative/path.txt", "file");
    check("file.txt", "file");
    check("", "file");
    check("/", "file");
    check("//server/share/file.txt", "file");          // UNC-ish, still a local path form
    check("/home/user/weird:name.txt", "file");        // a ':' inside a name is legal on POSIX
    check("/home/user/a:/b", "file");                  // ...even followed by a '/'
    check("http", "file");                             // letters only, no ':' at all
    check("http:", "file");                            // scheme punctuation but no '/'
    check(":/notascheme", "file");                     // no letters before the ':'
    check("a:/x", "file");                             // ONE letter: a Windows drive, not a scheme

    // --- Windows drive letters must NEVER read as a protocol ------------------------------
    check("C:/Users/test/file.txt", "file");
    check("c:/users/test", "file");
    check("C:\\Users\\test\\file.txt", "file");        // backslashes: not even a ":/" pair
    check("Z:/", "file");

    // --- real protocols -------------------------------------------------------------------
    check("sftp://root@127.0.0.1/XXX", "sftp");
    check("smb://server/share/file", "smb");
    check("ftp://ftp.example.org/pub", "ftp");
    check("fish://host/path", "fish");
    check("webdavs://host/path", "webdavs");
    check("mtp://device/DCIM", "mtp");
    check("sftp:/single-slash-still-a-scheme", "sftp");

    // --- "file" URIs stay "file" (the file managers hand us these all day) ------------------
    check("file:///home/user/file.txt", "file");
    check("file://192.168.0.99/share/file.txt", "file");
    check("file:/home/user/file.txt", "file");

    // --- case: a scheme is case-insensitive, so it must be lower-cased ---------------------
    check("SFTP://root@host/x", "sftp");
    check("Sftp://root@host/x", "sftp");
    check("FILE:///home/user/x", "file");

    // --- length-delimited: an embedded NUL must not confuse the scan ------------------------
    // (paths are carried as std::string precisely so a 0x00 does not truncate them)
    std::string nulPath("sftp://host/a");
    nulPath.push_back('\0');
    nulPath += "b";
    if (extractProtocol(nulPath) != "sftp") {
        ++failures;
        std::cout << "FAIL: a NUL inside the path changed the protocol" << std::endl;
    }
    std::string nulLocal("/home/user/a");
    nulLocal.push_back('\0');
    nulLocal += "sftp://x";
    if (extractProtocol(nulLocal) != "file") {
        ++failures;
        std::cout << "FAIL: a scheme AFTER a NUL was picked up" << std::endl;
    }

    // --- a very long scheme-looking prefix must not run off the end -------------------------
    check(std::string(4096, 'a'), "file");
    check(std::string(4096, 'a') + "://x", std::string(4096, 'a'));

    if (failures == 0)
        std::cout << "== protocol_test: all checks passed" << std::endl;
    else
        std::cout << "== protocol_test: " << failures << " failure(s)" << std::endl;
    return failures == 0 ? 0 : 1;
}
