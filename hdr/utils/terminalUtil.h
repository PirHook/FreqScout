#include <iostream>
#include <cstdlib>
#include <array>
#include <string>
#include <memory>
#include <stdexcept>
#include <system_error>

std::pair<std::string, int> exec(const std::string& cmd){
    std::string out;
    std::array<char, 1024> buffer;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(),"r"), pclose);

    if(!pipe){
        std::cerr << "Could not open pipe!\n";
        return {std::string(), -1};
    }

    while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr){
        out += buffer.data();
    }

    int exitCode = -1;
    auto status = pclose(pipe.release());
    if(WIFEXITED(status)){
        exitCode = WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status)){
        exitCode = -WTERMSIG(status);
    }

    return {std::move(out), exitCode};
}
