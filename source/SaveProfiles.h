#pragma once
#include "People.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace saveprofiles {
namespace fs=std::filesystem;
enum class Status { Fresh, Loaded, Recovered, Corrupt };
struct Loaded {people::Registry world;std::string key;Status status=Status::Fresh;};
// Two independent 64-bit streaming fingerprints plus file length. This is an identity
// for accidental save-file replacement, not a security or authenticity check.
inline std::string fingerprint(const fs::path &file) {
    std::ifstream in(file,std::ios::binary);
    if(!in)throw std::runtime_error("GTA save file cannot be read");
    std::uint64_t a=14695981039346656037ULL,b=7809847782465536322ULL,size=0;
    // Identical files in different save slots still represent separate branches.
    auto slot=file.filename().generic_u8string();
    for(auto c:slot) {
        const auto byte=static_cast<unsigned char>(c>='A'&&c<='Z'?c-'A'+'a':c);
        a=(a^byte)*1099511628211ULL;b=(b^(byte+0x9du))*14029467366897019727ULL;
    }
    a^=0xffu;b^=0x71u;
    char buffer[8192];
    while(in.read(buffer,sizeof(buffer)) || in.gcount()) {
        const auto n=in.gcount();size+=static_cast<std::uint64_t>(n);
        if(size>32*1024*1024)throw std::runtime_error("Unexpected GTA save file size");
        for(std::streamsize i=0;i<n;++i) {
            const auto c=static_cast<unsigned char>(buffer[i]);
            a=(a^c)*1099511628211ULL;b=(b^(c+0x9du))*14029467366897019727ULL;
        }
    }
    if(in.bad() || size<16)throw std::runtime_error("GTA save fingerprint failed");
    b^=size;
    std::ostringstream out;out<<std::hex<<std::setfill('0')<<std::setw(16)<<a<<std::setw(16)<<b;
    return out.str();
}
inline bool validKey(const std::string &key) {
    return key.size()==32 && std::all_of(key.begin(),key.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});
}
inline fs::path checkpoint(const fs::path &root,const std::string &key) {
    if(!validKey(key))throw std::runtime_error("Invalid mod save identity");
    return root/L"worlds"/fs::path(key)/L"checkpoint.dat";
}
inline fs::path live(const fs::path &root,const std::string &key) {
    if(!key.empty() && !validKey(key))throw std::runtime_error("Invalid live save identity");
    return root/L"worlds"/fs::path(key.empty()?"new-game":key)/L"live.dat";
}
inline void atomicWrite(const fs::path &target,const people::Registry &world) {
    // Serialize/validate first: a bad in-memory state must not touch an existing archive.
    std::ostringstream serialized;world.write(serialized);
    fs::create_directories(target.parent_path());
    auto temp=target;temp+=L".tmp";auto backup=target;backup+=L".bak";
    {
        std::ofstream out(temp,std::ios::binary|std::ios::trunc);
        const auto bytes=serialized.str();out.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));out.flush();
        if(!out)throw std::runtime_error("Mod save flush failed");
    }
    if(fs::exists(target))fs::copy_file(target,backup,fs::copy_options::overwrite_existing);
#ifdef _WIN32
    if(!MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Mod save replacement failed");
#else
    fs::rename(temp,target);
#endif
}
inline people::Registry read(const fs::path &file) {
    std::ifstream in(file,std::ios::binary);if(!in)throw std::runtime_error("Mod checkpoint cannot be read");
    return people::Registry::read(in);
}
inline Loaded load(const fs::path &root,const fs::path &gameSave) {
    Loaded result;result.key=fingerprint(gameSave);
    const auto file=checkpoint(root,result.key);
    auto backup=file;backup+=L".bak";
    if(!fs::exists(file) && !fs::exists(backup))return result;
    try {result.world=read(file);result.status=Status::Loaded;}
    catch(const std::exception &) {
        try {result.world=read(backup);result.status=Status::Recovered;}
        catch(const std::exception &){result.status=Status::Corrupt;}
    }
    return result;
}
inline std::string save(const fs::path &root,const fs::path &gameSave,const people::Registry &world) {
    const auto key=fingerprint(gameSave);atomicWrite(checkpoint(root,key),world);return key;
}
inline bool mayImport(const fs::path &root,const std::string &key) {
    return (key.empty() || validKey(key)) && fs::is_regular_file(root/L"people.dat");
}
inline bool importLegacy(const fs::path &root,const std::string &key,bool confirmed,people::Registry &out) {
    if(!confirmed || !mayImport(root,key))return false;
    auto legacy=read(root/L"people.dat");
    // Preserve the shared input verbatim; never guess which old actions belonged to which save.
    const auto backup=root/L"people.pre-save-profiles.dat";
    if(!fs::exists(backup))fs::copy_file(root/L"people.dat",backup);
    if(!key.empty())atomicWrite(checkpoint(root,key),legacy);
    out=std::move(legacy);return true;
}
}
