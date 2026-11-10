#include "../../hdr/gui/MainWindow.h"

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

    case State::eSTART_SPECTRUM:
      BuildAnalyzingFrame();
      break;
    case State::eSPECTRUM_RUNNING:
      BuildSpectrumFrame();
      BuildWaterfall();
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

void MainWindow::BuildAnalyzingFrame() { ImGui::Text("SPECTRUM COMIG SOON\n"); }

void MainWindow::BuildSpectrumFrame() {
  if (ImGui::Button("<- Back to Stations", ImVec2(160, 30))) {
    _stateMgr.SetState(State::eSPECTRUM_STOP);
    return;
  }

  if (!_context._currentSpectrumData.empty() &&
      _context._selectedStation.has_value()) {
    const float left_margin = 65.0f;
    const float right_margin = 20.0f;
    const float graph_height = 300.0f;

    ImVec2 canvas_size =
        ImVec2(ImGui::GetContentRegionAvail().x - left_margin - right_margin,
               graph_height);
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 startPos = ImGui::GetCursorScreenPos();

    draw_list->AddRectFilled(ImVec2(startPos.x + left_margin, startPos.y),
                             ImVec2(startPos.x + left_margin + canvas_size.x,
                                    startPos.y + canvas_size.y),
                             ImColor(10, 15, 25, 255));

    float dbSteps[] = {0.0f, -20.0f, -40.0f, -60.0f, -80.0f, -100.0f};
    for (float db : dbSteps) {
      float normY = 1.0f - (db - (-110.0f)) / (0.0f - (-110.0f));
      float yPos = startPos.y + normY * canvas_size.y;

      draw_list->AddLine(ImVec2(startPos.x + left_margin, yPos),
                         ImVec2(startPos.x + left_margin + canvas_size.x, yPos),
                         ImColor(255, 255, 255, 30));

      char db_buf[16];
      snprintf(db_buf, sizeof(db_buf), "%d", (int)db);
      draw_list->AddText(ImVec2(startPos.x + 10, yPos - 7),
                         ImColor(180, 180, 180), db_buf);
    }

    float centerFreq = std::stof(_context._selectedStation.value().freq);
    float bandwidth = 2.0f;
    float halfBw = bandwidth / 2.0f;
    float stepSizeMhz = 0.2f;
    float startGridFreq =
        ceilf((centerFreq - halfBw) / stepSizeMhz) * stepSizeMhz;

    for (float f = startGridFreq; f <= (centerFreq + halfBw);
         f += stepSizeMhz) {
      float normX = (f - (centerFreq - halfBw)) / bandwidth;
      float xPos = startPos.x + left_margin + normX * canvas_size.x;

      if (xPos < startPos.x + left_margin ||
          xPos > startPos.x + left_margin + canvas_size.x)
        continue;

      draw_list->AddLine(ImVec2(xPos, startPos.y),
                         ImVec2(xPos, startPos.y + canvas_size.y),
                         ImColor(255, 255, 255, 30));

      if (std::abs(fmod(f + 0.001f, 0.4f)) < 0.05f) {
        char fLabel[16];
        snprintf(fLabel, sizeof(fLabel), "%.1f", f);
        draw_list->AddText(ImVec2(xPos - 15, startPos.y + canvas_size.y + 5),
                           ImColor(150, 150, 150), fLabel);
      }
    }

    ImGui::SetCursorScreenPos(ImVec2(startPos.x + left_margin, startPos.y));
    ImGui::PlotLines("##Spectrum", _context._currentSpectrumData.data(),
                     _context._currentSpectrumData.size(), 0, nullptr, -110.0f,
                     0.0f, canvas_size);

    float centerX = startPos.x + left_margin + canvas_size.x * 0.5f;
    draw_list->AddLine(ImVec2(centerX, startPos.y),
                       ImVec2(centerX, startPos.y + canvas_size.y),
                       ImColor(255, 40, 40, 200), 2.0f);

    ImGui::SetCursorScreenPos(
        ImVec2(startPos.x, startPos.y + canvas_size.y + 40));
  }
}

void MainWindow::BuildWaterfall() {
  if (_context._currentWaterfall.empty())
    return;

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  ImVec2 startPos = ImGui::GetCursorScreenPos();

  const float left_margin = 65.0f;
  const float right_margin = 20.0f;
  float wf_width =
      ImGui::GetContentRegionAvail().x - left_margin - right_margin;
  float wf_height = 250.0f;

  const float minDb = -100.0f;
  const float maxDb = -30.0f;

  float l_bar_width = 10.0f;
  float l_x_pos = startPos.x + 40.0f;

  draw_list->AddText(ImVec2(startPos.x + 10, startPos.y - 20),
                     ImColor(200, 200, 200), "dB");

  for (int i = 0; i < 100; i++) {
    float intensity = 1.0f - ((float)i / 100.0f);
    ImU32 col = ImGui::ColorConvertFloat4ToU32(
        ImVec4(intensity, intensity * 0.8f, 1.0f - intensity, 1.0f));
    draw_list->AddRectFilled(
        ImVec2(l_x_pos, startPos.y + i * (wf_height / 100.0f)),
        ImVec2(l_x_pos + l_bar_width,
               startPos.y + (i + 1) * (wf_height / 100.0f)),
        col);
  }

  float labels[] = {maxDb, (maxDb + minDb) / 2.0f, minDb};
  for (int i = 0; i < 3; i++) {
    float y_off = (i / 2.0f) * wf_height;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)labels[i]);

    draw_list->AddText(ImVec2(l_x_pos - 30, startPos.y + y_off - 7),
                       ImColor(150, 150, 150), buf);
  }

  float wf_x_start = startPos.x + left_margin;
  const int bins = 1024;
  const int history = 100;
  float col_w = wf_width / (float)bins;
  float row_h = wf_height / (float)history;

  for (int y = 0; y < history; y++) {
    for (int x = 0; x < bins; x++) {
      float val = _context._currentWaterfall[y * bins + x];
      float intensity = std::clamp((val - minDb) / (maxDb - minDb), 0.0f, 1.0f);

      ImU32 color = ImGui::ColorConvertFloat4ToU32(
          ImVec4(intensity, intensity * 0.8f, 1.0f - intensity, 1.0f));
      draw_list->AddRectFilled(
          ImVec2(wf_x_start + x * col_w, startPos.y + y * row_h),
          ImVec2(wf_x_start + (x + 1) * col_w, startPos.y + (y + 1) * row_h),
          color);
    }
  }

  float centerX = wf_x_start + wf_width * 0.5f;
  draw_list->AddLine(ImVec2(centerX, startPos.y),
                     ImVec2(centerX, startPos.y + wf_height),
                     ImColor(255, 255, 255, 120), 1.0f);

  ImGui::Dummy(ImVec2(0, wf_height + 10.0f));
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
        _stateMgr.SetState(State::eSTART_SPECTRUM);
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
