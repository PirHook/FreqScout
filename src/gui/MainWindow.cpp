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
    if (ImGui::Button("Start Scan", ImVec2(120, 40))) {
      _isScanning = true;
      _gsmScanner.Start();
    }
  } else {
    if (ImGui::Button("Stop Scan", ImVec2(120, 40))) {
      _isScanning = false;
      _gsmScanner.Stop();
    }

    ImGui::Separator();

    // Опрос сканера (раз в 2 кадра, как ты и хотел)
    if (_frameCounter % 2 == 0) {
      _stations = _gsmScanner.GetStations();
    }

    if (!_stations.empty()) {
      // Флаги: границы, чередование цветов строк, возможность менять ширину
      // колонок
      static ImGuiTableFlags tableFlags =
          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
          ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;

      // Запускаем таблицу
      if (ImGui::BeginTable("GsmStationsTable", 7, tableFlags)) {
        // Настройка заголовков
        ImGui::TableSetupColumn("ARFCN");
        ImGui::TableSetupColumn("Frequency");
        ImGui::TableSetupColumn("CID");
        ImGui::TableSetupColumn("LAC");
        ImGui::TableSetupColumn("MCC");
        ImGui::TableSetupColumn("MNC");
        ImGui::TableSetupColumn("Pwr (dBm)");
        ImGui::TableHeadersRow();

        for (const auto &s : _stations) {
          ImGui::TableNextRow();

          ImGui::TableSetColumnIndex(0);
          ImGui::Text("%d", s.arfcn);
          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(s.freq.c_str());
          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%d", s.cid);
          ImGui::TableSetColumnIndex(3);
          ImGui::Text("%d", s.lac);
          ImGui::TableSetColumnIndex(4);
          ImGui::TextUnformatted(s.mcc.c_str());
          ImGui::TableSetColumnIndex(5);
          ImGui::TextUnformatted(s.mnc.c_str());

          // Цветовой индикатор для Power
          ImGui::TableSetColumnIndex(6);
          if (s.pwr > -75)
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%d", s.pwr);
          else
            ImGui::Text("%d", s.pwr);
        }
        ImGui::EndTable();
      }
    } else {
      ImGui::TextColored(ImVec4(1, 1, 0, 1), "Searching for GSM towers...");
    }
  }

  ImGui::End();
}
