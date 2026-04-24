#pragma once

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>
#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../utils/Scanner.h"

struct WindowDeleter {
  void operator()(SDL_Window *w) const { SDL_DestroyWindow(w); }
};

struct RendererDeleter {
  void operator()(SDL_Renderer *r) const { SDL_DestroyRenderer(r); }
};

class MainWindow {
  MainWindow(const MainWindow &) = delete;
  MainWindow &operator=(const MainWindow &) = delete;

public:
  MainWindow();
  ~MainWindow();

  void Run();

private:
  void DrawNextFrame();
  void BuildMainThemeFrame();

private:
  const int16_t _winWidth = 800;
  const int16_t _winHeight = 480;
  uint16_t _frameCounter;
  bool _running;
  bool _isScanning;
  ImGuiWindowFlags _winFlags;

  std::unique_ptr<SDL_Window, WindowDeleter> _window;
  std::unique_ptr<SDL_Renderer, RendererDeleter> _renderer;
  std::vector<Station> _stations;

  Scanner _gsmScanner;
};
