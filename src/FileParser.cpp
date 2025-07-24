#include "FileParser/Cli.h"
#include "FileParser/Gui/Renderer.h"

int main() {
    Gui::Renderer::Init();
    Cli::RunCli();
    return 0;
}