#include "../maintenance/reset_files.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
int main(){
    namespace fs=std::filesystem;
    char root[]="/tmp/pg-reset-XXXXXX";
    if(!::mkdtemp(root))return 1;
    const std::string dir=std::string(root)+"/world",id(32,'a');
    try{
        fs::create_directory(dir);
        auto put=[](const std::string& path,const std::string& text){std::ofstream f(path);f<<text;if(!f)throw std::runtime_error("fixture write failed");};
        put(dir+"/identity.pgim",id);put(dir+"/world-a.pgid","A");put(dir+"/world-b.pgid","B");put(std::string(root)+"/unrelated","keep");
        genesis::maintenance::ResetFiles io{dir,[&](){std::ifstream f(dir+"/identity.pgim");std::string s;f>>s;return s;}};
        put(dir+"/unknown","keep");bool refused=false;try{io.preflight();}catch(const std::runtime_error&){refused=true;}assert(refused&&!io.pending());fs::remove(dir+"/unknown");
        io.begin_reset(id);assert(io.pending()&&io.reset_identity()==id);
        put(dir+"/identity.pgim",std::string(32,'b'));refused=false;try{io.erase_world_files(id);}catch(const std::runtime_error&){refused=true;}assert(refused&&fs::exists(dir+"/world-a.pgid"));
        put(dir+"/identity.pgim",id);fs::remove(dir+"/world-a.pgid");io.erase_world_files(id);assert(io.world_absent()&&io.pending());io.finish_reset();assert(!io.pending()&&fs::exists(std::string(root)+"/unrelated"));
        fs::remove_all(root);std::cout<<"PASS allowlist, marker, foreign identity, partial deletion and unrelated file preservation\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';fs::remove_all(root);return 1;}
}
