#include "../../hdr/gui/MainWindow.h"

MainWindow::MainWindow()
    : _running(true), _lastTick(SDL_GetTicks()), _fps(0), _fpsTimer(0.0f),
      _fpsFrames(0), _scanTimer(0.0f) {

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

    // -------- FPS --------
    _fpsTimer += dt;
    _fpsFrames++;

    if (_fpsTimer >= 1.0f) {
      _fps = _fpsFrames;
      _fpsFrames = 0;
      _fpsTimer = 0.0f;
    }

    // -------- Logic --------
    Update(dt);

    // -------- ImGui frame --------
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(_winWidth, _winHeight));

    ImGui::Begin("Control Panel", nullptr, _winFlags);

    ImGui::Text("FreqScout. GSM Signal Analyze utility");
    ImGui::Text("FPS: %d", _fps);

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

    case State::eSCAN_DONE_DATA_ACQUIRED:
      BuildRestartFrame();
      DrawStationTable();
      break;

    case State::eSCAN_DONE_NO_DATA:
      BuildRestartFrame();
      break;

    default:
      break;
    }

    ImGui::End();

    DrawNextFrame();

    SDL_Delay(8); // минимальный yield CPU
  }
}

void MainWindow::Update(float dt) {
  switch (_stateMgr.GetState()) {

  case State::eSCAN_STARTED:
    if (!_gsmScanner.GetState()) {
      _gsmScanner.Start();
    }
    _stateMgr.SetState(State::eSCANNING);
    break;

  case State::eSCANNING:
    _scanTimer += dt;

    if (_scanTimer >= 0.1f) { // 10 Hz
      _stations = _gsmScanner.GetStations();
      _scanTimer = 0.0f;
    }

    if (!_stations.empty()) {
      _stateMgr.SetState(State::eSCANNING_DATA_ACQUIRED);
    }
    break;

  case State::eSCANNING_DATA_ACQUIRED:
    _scanTimer += dt;

    if (_scanTimer >= 0.1f) {
      _stations = _gsmScanner.GetStations();
      _scanTimer = 0.0f;
    }
    break;

  case State::eSCAN_STOPPED:
    if (_stations.empty()) {
      _stateMgr.SetState(State::eSCAN_DONE_NO_DATA);
    } else {
      _stateMgr.SetState(State::eSCAN_DONE_DATA_ACQUIRED);
    }
    _gsmScanner.Stop();
    break;

  default:
    break;
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
    _stateMgr.SetState(State::eSCAN_STOPPED);
    return;
  }

  ImGui::Separator();

  if (_stations.empty()) {
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Searching for GSM towers...");
  }
}

void MainWindow::DrawStationTable() {
  if (_stations.empty())
    return;

  static ImGuiTableFlags tableFlags =
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;

  if (ImGui::BeginTable("GsmStationsTable", 7, tableFlags)) {

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

      ImGui::TableSetColumnIndex(6);
      if (s.pwr > -75)
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%d", s.pwr);
      else
        ImGui::Text("%d", s.pwr);
    }

    ImGui::EndTable();
  }
}
