#include <iostream>

#include "Cli.h"
#include "Gui/Renderer.h"

// TODO: Separate renderer and conversion functions into separate modules
int main() {
    Gui::Renderer::Init();
    Cli::RunCli();
    std::cout << "End main\n";
    return 0;
}