#include "window.hpp"

int main() {

    auto window = Window::createDefaultWindow();

    window.setClearColor(1, 0, 0);

    window.show();

    while(!window.shouldShutdown()) {
        window.pollEvent();
        window.update();
    }

    window.hide();
}