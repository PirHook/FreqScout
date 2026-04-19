#include "../../hdr/gui/MainWindow.h"

MainWindow::MainWindow()
    : _running(true), _frameCounter(0), _isScanning(false) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "SDL init error\n";
    std::terminate();
  }

  _window.reset(SDL_CreateWindow("FreqScout", SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED, _winWidth, _winHeight,
                                 0));

  if (!_window) {
    std::cout << "Can't create Window!\n";
    std::terminate();
  }

  _renderer.reset(
      SDL_CreateRenderer(_window.get(), -1, SDL_RENDERER_ACCELERATED));

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplSDL2_InitForSDLRenderer(_window.get(), _renderer.get());
  ImGui_ImplSDLRenderer2_Init(_renderer.get());

  _winFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
              ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
}

MainWindow::~MainWindow() {
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_Quit();
}

void MainWindow::Run() {
  while (_running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT) {
        _running = false;
      }
    }

    _frameCounter == 1024 ? _frameCounter = 0 : _frameCounter;

    BuildMainThemeFrame();

    DrawNextFrame();

    _frameCounter++;

    SDL_Delay(8);
  }
}

void MainWindow::DrawNextFrame() {
  ImGui::Render();
  SDL_SetRenderDrawColor(_renderer.get(), 0, 0, 0, 255);
  SDL_RenderClear(_renderer.get());
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), _renderer.get());
  SDL_RenderPresent(_renderer.get());
}

void MainWindow::BuildMainThemeFrame() {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(_winWidth, _winHeight));
  ImGui::Begin("Control Panel", nullptr, _winFlags);

  ImGui::Text("FreqScout. GSM Signal Analyze utility");

  if (!_isScanning) {
    if (ImGui::Button("Start Scan")) {
      _isScanning = true;
      _gsmScanner.Start();
    }
  } else {
    ImGui::Separator();
    ImGui::BeginChild("Output", ImVec2(0, 0), true);
    if (!_frameCounter % 2) {
      _winContext = _gsmScanner.GetOutput().c_str();
    }
    ImGui::TextUnformatted(_winContext.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
      ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
  }

  ImGui::End();
}
