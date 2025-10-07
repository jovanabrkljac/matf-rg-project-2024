
#include <GuiController.hpp>
#include <MainController.hpp>
#include <MyApp.hpp>
#include <engine/core/ProcessController.hpp>

#include <spdlog/spdlog.h>

namespace app {
void MyApp::app_setup() {
    spdlog::info("App setup completed!");
    auto main_controller = register_controller<app::MainController>();
    auto gui_controller = register_controller<app::GUIController>();
    register_controller<engine::core::ProcessController>();

    main_controller->after(engine::core::Controller::get<engine::core::EngineControllersEnd>());
    main_controller->before(gui_controller);
}
}// app