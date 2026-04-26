#include "../../hdr/gui/MainWindow.h"
#include <rtl-sdr.h>

MainWindow::MainWindow() : _running(true), _lastTick(SDL_GetTicks()) {

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

    // -------- Time --------
    uint32_t now = SDL_GetTicks();
    float dt = (now - _lastTick) / 1000.0f;
    _lastTick = now;

    // -------- Logic --------
    _stateMgr.Update(dt, _context);

    // -------- ImGui frame --------
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(_winWidth, _winHeight));

    ImGui::Begin("Control Panel", nullptr, _winFlags);

    ImGui::Text("FreqScout. GSM Signal Analyze utility");
    ImGui::Text("FPS: %d", _stateMgr.GetFps());

    switch (_stateMgr.GetState()) {
    case State::eMAIN_THEME:
      BuildMainThemeFrame();
      break;

    case State::eSCANNING:
      BuildScanFrame();
      break;

    case State::eSCANNING_DATA_ACQUIRED:
      BuildScanFrame();
      DrawStationTable();
      break;

    case State::eSCAN_DONE:
      BuildRestartFrame();
      DrawStationTable();
      break;

    case State::eANALYZING:
      BuildAnalyzingFrame();
      break;

    default:
      break;
    }

    ImGui::End();

    DrawNextFrame();

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
  if (ImGui::Button("Start Scan", ImVec2(120, 40))) {
    _stateMgr.SetState(State::eSCAN_STARTED);
  }
}

void MainWindow::BuildRestartFrame() {
  if (ImGui::Button("Restart Scan", ImVec2(120, 40))) {
    _stateMgr.SetState(State::eSCAN_STARTED);
  }
}

void MainWindow::BuildScanFrame() {
  if (ImGui::Button("Stop Scan", ImVec2(120, 40))) {
    _stateMgr.SetState(State::eSCAN_DONE);
    return;
  }

  ImGui::Separator();

  if (_context._stations.empty()) {
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Searching for GSM towers...");
  }
}

void MainWindow::BuildAnalyzingFrame() {
  ImGui::Text("SPECTRUM COMIG SOON: %d\n", rtlsdr_get_device_count());
}

void MainWindow::DrawStationTable() {
  if (_context._stations.empty())
    return;

  static ImGuiTableFlags tableFlags =
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;

  if (ImGui::BeginTable("GsmStationsTable", 8, tableFlags)) {

    ImGui::TableSetupColumn("");
    ImGui::TableSetupColumn("ARFCN");
    ImGui::TableSetupColumn("Frequency");
    ImGui::TableSetupColumn("CID");
    ImGui::TableSetupColumn("LAC");
    ImGui::TableSetupColumn("MCC");
    ImGui::TableSetupColumn("MNC");
    ImGui::TableSetupColumn("Pwr (dBm)");
    ImGui::TableHeadersRow();

    for (const auto &s : _context._stations) {
      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      std::string btnId = "Analyze##" + std::to_string(s.arfcn);
      if (ImGui::Button(btnId.c_str())) {
        _context._selectedStation = s;
        _stateMgr.SetState(State::eANALYZING);
      }

      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%d", s.arfcn);

      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(s.freq.c_str());

      ImGui::TableSetColumnIndex(3);
      ImGui::Text("%d", s.cid);

      ImGui::TableSetColumnIndex(4);
      ImGui::Text("%d", s.lac);

      ImGui::TableSetColumnIndex(5);
      ImGui::TextUnformatted(s.mcc.c_str());

      ImGui::TableSetColumnIndex(6);
      ImGui::TextUnformatted(s.mnc.c_str());

      ImGui::TableSetColumnIndex(7);
      if (s.pwr > -75)
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%d", s.pwr);
      else
        ImGui::Text("%d", s.pwr);
    }

    ImGui::EndTable();
  }
}
