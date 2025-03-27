#include <iostream>

#include "Cli.h"
#include "Gui/Renderer.h"

int main() {
    Gui::Renderer::Init();
    Cli::RunCli();
    return 0;
}