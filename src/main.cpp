#include "Application.h"
#include <iostream>

// Declare the openFileDialog function (implemented elsewhere, e.g., file_dialog_mac.mm)
// It might be better to move this into the UIManager or Application class if needed.
// std::string openFileDialog(); // Commented out for now as it's used within UIManager

int main() {
    Application app;

    if (!app.initialize()) {
        std::cerr << "Application initialization failed!" << std::endl;
        return -1;
    }

    app.run();

    // Cleanup is handled in Application destructor

    return 0;
}