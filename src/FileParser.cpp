#include <iostream>

#include "Cli.h"
#include "Renderer.h"

// TODO: Separate renderer and conversion functions into separate modules
int main(const int argc, char* argv[]) {
    Renderer::Init();
    cli::RunCli();
    std::cout << "End main\n";
    return 0;
}