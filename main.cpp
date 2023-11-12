#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include<vector>

//По максимуму решил убрать if-else во имя быстродействия чтоб процессор
//по минимуму оставлять в "подвешенном" состоянии

using namespace std;
using filesystem::path;

pair<bool, path> GlobalSearch(const string& what_find, const vector<path>& include_directories);
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories);
void MatchedDirs(const vector<path>& original, vector<path>& matched);
void Muter() {} //используется как заглушка

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

string GetFileContents(string file) {
    ifstream stream(file);
    // конструируем string по двум итераторам
    return { (istreambuf_iterator<char>(stream)), istreambuf_iterator<char>() };
}


pair<bool, path> LocalSearch(const string& what_find, const path& where_find,
    const vector<path>& include_directories) {

    switch (exists(where_find))
    {
    case true:
        for (const filesystem::directory_entry& entry : filesystem::directory_iterator(where_find)) {

            switch (entry.path().filename().string() == what_find) {
            case true:
                return make_pair(true, entry.path());
                break;
            }
        }
        break;
    }
    return GlobalSearch(what_find, include_directories);
}



void MatchedDirs(const vector<path>& original, vector<path>& matched) {
    vector<path>to_loop;
    for (const path& pth : original) {
        switch (filesystem::directory_entry(pth).is_directory())
        {
        case true:
            for (const filesystem::directory_entry& entry : filesystem::directory_iterator(pth)) {
                entry.is_regular_file() ? matched.push_back(entry.path()) : to_loop.push_back(entry.path());
            };
            break;
        default:
            matched.push_back(pth);
            break;
        }
    }
    !to_loop.empty() ? MatchedDirs(to_loop, matched) : Muter();
}


pair<bool, path> GlobalSearch(const string& what_find, const vector<path>& include_directories) {
   //Пытался решить задачу используя рекурсивнвй итератор - в IDE все хорошо - компилятор практикума не принимает
    //пришлось делать Matched_dirs
    
    
    vector<path> matched_directions;
    MatchedDirs(include_directories, matched_directions);
    for (const path& pth : matched_directions) {

        switch (pth.filename().string() == what_find) {
        case true:
            return make_pair(true, pth);
            break;
        }
    }
    return make_pair(false, "");
}

pair<string, path> ConstructorPath(path parent_path, string modificator)
{
    path temp_path = parent_path / path(modificator);
    path where_find = temp_path.parent_path();
    string what_find = temp_path.filename().string();
    return make_pair(what_find, where_find);
}

void PrintError(const string& what, const string& whereis, int line = 0) {
    cout << "unknown include file " << what << " at file " << whereis << " at line " << line << endl;
}


bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {

    if (!filesystem::exists(in_file)) {
        return false;
    }


    if (filesystem::directory_entry(in_file).is_directory()) {
        vector<path> direct_paths;
        for (const filesystem::directory_entry& entry : filesystem::directory_iterator(in_file)) {
            direct_paths.push_back(entry.path());
        }
        vector<path>matched_paths;
        MatchedDirs(direct_paths, matched_paths);
        for (const path& pth : matched_paths) {
            Preprocess(pth, out_file, include_directories);
        }
    };

    int counter = 0;
    ifstream readln(in_file, ios::binary);
    if (!readln) {
        return false;
    };

    if (filesystem::directory_entry(in_file).is_directory())
    {
        for (const filesystem::directory_entry& entry : filesystem::directory_iterator(in_file)) {
            Preprocess(entry.path(), out_file, include_directories);
        }
    }

    const  regex R1(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    const  regex R2(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
    path par_path = in_file.parent_path();
    string file_name = in_file.filename().string();
    smatch sm;
    if (filesystem::directory_entry(in_file).is_regular_file()) {

         
        
        
        
        do
        {
            string buf;
            if (!getline(readln, buf))break;
            ++counter;

            enum CHOISE {
                LOCAL, GLOBAL, DEFAULT
            };

            CHOISE choise;
            regex_match(buf, sm, R1) ? choise = LOCAL : regex_match(buf, sm, R2) ? choise = GLOBAL : choise = DEFAULT ;
            pair <string, path> where_and_what;
            pair<bool, path> tup;
            
            switch (choise) {
            case LOCAL:
               where_and_what = ConstructorPath(par_path, sm[1]);
               tup = LocalSearch(where_and_what.first, where_and_what.second, include_directories);
                if (!tup.first) {
                    PrintError(where_and_what.first, in_file.string(), counter);
                    return false;
                }
                Preprocess(tup.second, out_file, include_directories);
            break;
            case GLOBAL:
                where_and_what = ConstructorPath(par_path, sm[1]);
                tup = GlobalSearch(where_and_what.first, include_directories);
                if (!tup.first) {
                    PrintError(where_and_what.first, in_file.string(), counter);
                    return false;
                }
                Preprocess(tup.second, out_file, include_directories);
            break;
            default:
                ofstream writeln(out_file, ios::binary | ios::app);
                writeln << buf << endl;
                writeln.close();
            break;
            }
        
           
        } while (readln);

    }
    return true;
}


void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file <<
            "// this comment before include\n"
            "#include \"dir1/b.h\"\n"
            "// text between b.h and c.h\n"
            "#include \"dir1/d.h\"\n"
            "\n"
            "int SayHello() {\n"
            "    cout << \"hello, world!\" << endl;\n"
            "#   include<dummy.txt>\n"
            "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
            "#include \"subdir/c.h\"\n"
            "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
            "#include <std1.h>\n"
            "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
            "#include \"lib/std2.h\"\n"
            "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }



    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
        { "sources"_p / "include1"_p,"sources"_p / "include2"_p })));

    ostringstream test_out;
    test_out << "// this comment before include\n"
        "// text from b.h before include\n"
        "// text from c.h before include\n"
        "// std1\n"
        "// text from c.h after include\n"
        "// text from b.h after include\n"
        "// text between b.h and c.h\n"
        "// text from d.h before include\n"
        "// std2\n"
        "// text from d.h after include\n"
        "\n"
        "int SayHello() {\n"
        "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    setlocale(LC_ALL, "Ru-ru");
    Test();
}