#pragma once

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>

#include <vector>

#include "../utils/AppContext.h"
#include "../utils/StateManager.h"

struct WindowDeleter {
  void operator()(SDL_Window *w) const { SDL_DestroyWindow(w); }
};

struct RendererDeleter {
  void operator()(SDL_Renderer *r) const { SDL_DestroyRenderer(r); }
};

class MainWindow {

public:
  MainWindow();
  ~MainWindow();

  void Run();

private:
  void Update(float dt);

  void DrawNextFrame();
  void BuildMainThemeFrame();
  void BuildRestartFrame();
  void BuildScanFrame();
  void BuildAnalyzingFrame();

  void DrawStationTable();

private:
  const int16_t _winWidth = 800;
  const int16_t _winHeight = 480;
  double _lastTick;
  bool _running;
  bool _isScanning;
  ImGuiWindowFlags _winFlags;

  std::unique_ptr<SDL_Window, WindowDeleter> _window;
  std::unique_ptr<SDL_Renderer, RendererDeleter> _renderer;
  std::vector<Station> _stations;

  StateManager _stateMgr;
  AppContext _context;
};
