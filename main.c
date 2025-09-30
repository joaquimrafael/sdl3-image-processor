/*
Projeto 1 (Proj1) – Processamento de imagens
Computação Visual, Ciência da Computação Mackenzie, Turma 07N 2025.2
Grupo:
Antonio Carlos Sciamarelli Neto - 10409160
Gustavo Matta - 10410154
Joaquim Rafael Mariano Prieto Pereira - 10408805
Lucas Trebacchetti Eiras - 10401973
*/

// Includes

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <math.h>

// Constants

static const char *WINDOW_TITLE = "Tela Principal";
static const char *WINDOW_TITLE2 = "Tela Secundária";
static const char *BUTTON_ORIGINAL = "Original";
static const char *BUTTON_EQUALIZED = "Equalizado";
char *IMAGE_FILENAME;
static const char *DEFAULT_OUTPUT_FILENAME = "output.png";

enum constants
{
  DEFAULT_WINDOW_WIDTH = 0,
  DEFAULT_WINDOW_HEIGHT = 0,
  DEFAULT_WINDOW_CHILD_WIDTH = 320,
  DEFAULT_WINDOW_CHILD_HEIGHT = 240,
};

typedef struct MyWindow MyWindow;
struct MyWindow
{
  SDL_Window *window;
  SDL_Renderer *renderer;
};

typedef struct MyImage MyImage;
struct MyImage
{
  SDL_Surface *surface;
  SDL_Texture *texture;
  SDL_FRect rect;
};

typedef struct Button Button;
struct Button
{
    SDL_FRect rect;
    SDL_Color color_normal;
    SDL_Color color_hover;
    SDL_Color color_pressed;
    
    const char *text;
    SDL_Texture *text_texture;
    int text_w;
    int text_h;
    
    bool is_hovered;
    bool is_pressed;
    bool was_clicked;
};

typedef struct Histogram Histogram;
struct Histogram
{
    SDL_FRect rect;
};

// Global variables

float counterIntensity[256];
float counterIntensityEqualized[256];
bool equalized = false;
SDL_FRect histBars[256];
SDL_Surface *originalSurface;
SDL_Surface *equalizedSurface;
char *contrast;
char *brightness;

static MyWindow g_window = { .window = NULL, .renderer = NULL };
static MyWindow g_windowChild = {.window = NULL, .renderer = NULL};
static MyImage g_image = {
  .surface = NULL,
  .texture = NULL,
  .rect = { .x = 0.0f, .y = 0.0f, .w = 0.0f, .h = 0.0f }
};
static Button g_button = {
    .rect = {0, 0, 0, 0},
    .color_normal = {0, 0, 0, 0},
    .color_hover  = {0, 0, 0, 0},
    .color_pressed = {0, 0, 0, 0},
    .text = NULL,
    .text_texture = NULL,
    .text_w = 0,
    .text_h = 0,
    .is_hovered = false,
    .is_pressed = false,
    .was_clicked = false
};
static Histogram g_hist = {
    .rect = {0,0,0,0}
};

// Function declarations

static bool MyWindow_initialize(MyWindow *window, const char *title, int width, int height, SDL_WindowFlags window_flags);
static void MyWindow_destroy(MyWindow *window);
static void MyImage_destroy(MyImage *image);
static void createButton();
static void renderButton();
static void toggleButtonText();
static void loadHistogramButton();
static SDL_AppResult initialize();
static void loadImage(const char *filename, SDL_Renderer *renderer, MyImage *output_image);
static void render();
static void createHistogram();
static void renderHistogramBars();
static void countIntensity(SDL_Surface *surface);
static void equalize(SDL_Surface *surface);
static void createTextureSurface(SDL_Renderer *renderer);
static void analyzeImage(SDL_Surface *surface);
static void renderImageStats();

// Function implementations

bool MyWindow_initialize(MyWindow *window, const char *title, int width, int height, SDL_WindowFlags window_flags)
{
  SDL_Log("\tMyWindow_initialize(%s, %d, %d)", title, width, height);
  return SDL_CreateWindowAndRenderer(title, width, height, window_flags, &window->window, &window->renderer);
}

void MyWindow_destroy(MyWindow *window)
{
  SDL_Log(">>> MyWindow_destroy()");

  SDL_Log("\tDestruindo MyWindow->renderer...");
  SDL_DestroyRenderer(window->renderer);
  window->renderer = NULL;

  SDL_Log("\tDestruindo MyWindow->window...");
  SDL_DestroyWindow(window->window);
  window->window = NULL;

  SDL_Log("<<< MyWindow_destroy()");
}

void MyImage_destroy(MyImage *image)
{
  SDL_Log(">>> MyImage_destroy()");

  if (!image)
  {
    SDL_Log("\t*** Erro: Imagem inválida (image == NULL).");
    SDL_Log("<<< MyImage_destroy()");
    return;
  }

  if (image->texture)
  {
    SDL_Log("\tDestruindo MyImage->texture...");
    SDL_DestroyTexture(image->texture);
    image->texture = NULL;
  }

  if (image->surface)
  {
    SDL_Log("\tDestruindo MyImage->surface...");
    SDL_DestroySurface(image->surface);
    image->surface = NULL;
  }

  SDL_Log("\tRedefinindo MyImage->rect...");
  image->rect.x = image->rect.y = image->rect.w = image->rect.h = 0.0f;

  SDL_Log("<<< MyImage_destroy()");
}

static SDL_AppResult initialize(void)
{
  SDL_Log(">>> initialize()");

  SDL_Log("Inicializando variáveis...");

  for(int i=0;i<256;i++){
    counterIntensity[i]=0;
    counterIntensityEqualized[i]=0;
  }

  SDL_Log("\tIniciando SDL...");
  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    SDL_Log("\t*** Erro ao iniciar a SDL: %s", SDL_GetError());
    SDL_Log("<<< initialize()");
    return SDL_APP_FAILURE;
  }

  SDL_Log("\tCriando janela e renderizador...");
  if (!MyWindow_initialize(&g_window, WINDOW_TITLE, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0))
  {
    SDL_Log("\tErro ao criar a janela e/ou renderizador: %s", SDL_GetError());
    SDL_Log("<<< initialize()");
    return SDL_APP_FAILURE;
  }

  SDL_Log("\tCriando janela filho e renderizador...");
  if (!MyWindow_initialize(&g_windowChild, WINDOW_TITLE2, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0))
  {
    SDL_Log("\tErro ao criar a janela filho e/ou renderizador: %s", SDL_GetError());
    SDL_Log("<<< initialize()");
    return SDL_APP_FAILURE;
  }
  
  if(!SDL_SetWindowParent(g_windowChild.window, g_window.window)) 
  {
    SDL_Log("\tErro setar parentesco entre a janela filho e pai: %s", SDL_GetError());
    SDL_Log("<<< initialize()");
    return SDL_APP_FAILURE;
  }

  if(!TTF_Init())
  {
    SDL_Log("\t*** Erro ao inicializar SDL_ttf: %s", SDL_GetError());
    SDL_Log("<<< initialize()");
    return SDL_APP_FAILURE;
  }

  SDL_Log("<<< initialize()");
  return SDL_APP_CONTINUE;
}

static void shutdown(void)
{
  SDL_Log(">>> shutdown()");

  MyImage_destroy(&g_image);
  MyWindow_destroy(&g_window);
  MyWindow_destroy(&g_windowChild);

  SDL_Log("\tEncerrando SDL...");
  SDL_Quit();

  SDL_Log("<<< shutdown()");
}

static void render(void)
{
  SDL_SetRenderDrawColor(g_window.renderer, 128, 128, 128, 255);
  SDL_RenderClear(g_window.renderer);
  SDL_RenderTexture(g_window.renderer, g_image.texture, &g_image.rect, &g_image.rect);
  SDL_RenderPresent(g_window.renderer);

  SDL_SetRenderDrawColor(g_windowChild.renderer, 128, 128, 128, 255);
  SDL_RenderClear(g_windowChild.renderer);
  renderButton();
  renderHistogramBars();
  renderImageStats();
  SDL_RenderPresent(g_windowChild.renderer);
}

static void loop(void)
{
  SDL_Log(">>> loop()");

  SDL_Cursor *cursor_arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
  SDL_Cursor *cursor_hand  = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);

  bool mustRefresh = false;
  render();

  SDL_Event event;
  bool isRunning = true;
  bool last_hovered = false;

  while(isRunning)
  {
    float mouse_x_global, mouse_y_global;
    SDL_GetGlobalMouseState(&mouse_x_global, &mouse_y_global);
    int child_x, child_y;
    SDL_GetWindowPosition(g_windowChild.window, &child_x, &child_y);
    int mouse_x = mouse_x_global - child_x;
    int mouse_y = mouse_y_global - child_y;
    bool hovering = (mouse_x>=g_button.rect.x && mouse_x<=g_button.rect.x+g_button.rect.w &&
                     mouse_y>=g_button.rect.y && mouse_y<=g_button.rect.y+g_button.rect.h);
    if(hovering!=g_button.is_hovered)
    {
      g_button.is_hovered = hovering;
      SDL_SetCursor(hovering?cursor_hand:cursor_arrow);
      mustRefresh = true;
    }
    if(g_button.was_clicked)
    {
      toggleButtonText();
      equalized = !equalized;
      if(equalized){
        equalize(g_image.surface);
        countIntensity(equalizedSurface);
      }
      else{
        g_image.surface = SDL_ConvertSurface(originalSurface, SDL_PIXELFORMAT_RGBA32);
        countIntensity(originalSurface);
      }
      createTextureSurface(g_window.renderer);
      g_button.was_clicked = false;
      mustRefresh = true;
    }
    while(SDL_PollEvent(&event))
    {
      switch(event.type)
      {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
          if(event.window.windowID == SDL_GetWindowID(g_window.window) ||
             event.window.windowID == SDL_GetWindowID(g_windowChild.window))
            isRunning = false;
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
          if(g_button.is_hovered)
          {
          g_button.is_pressed = true;
          mustRefresh = true;
          }
          break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
          if(g_button.is_pressed && g_button.is_hovered)
            g_button.was_clicked = true;
            g_button.is_pressed = false;
            mustRefresh = true;
            break;

        case SDL_EVENT_KEY_DOWN:
          if(event.key.key == SDLK_S){
            SDL_ClearError();
            if (!IMG_SavePNG(g_image.surface, DEFAULT_OUTPUT_FILENAME)) {
              SDL_Log("Erro ao salvar a imagem: %s", SDL_GetError());
            } else {
              SDL_Log("Imagem salva como %s", DEFAULT_OUTPUT_FILENAME);
            }
          }
          break;
      }
    }
    if(mustRefresh)
    {
      render();
      mustRefresh = false;
    }
    SDL_Delay(10);
}
  SDL_DestroyCursor(cursor_arrow);
  SDL_DestroyCursor(cursor_hand);
  
  SDL_Log("<<< loop()");
}

void createWindow()
{
    int imageWidth = (int)g_image.rect.w;
    int imageHeight = (int)g_image.rect.h;

    int imageWidthChild = DEFAULT_WINDOW_CHILD_WIDTH;
    int imageWidthHeight = DEFAULT_WINDOW_CHILD_HEIGHT;
    
    if (imageWidth > DEFAULT_WINDOW_WIDTH || imageHeight > DEFAULT_WINDOW_HEIGHT)
    {
      int top = 0;
      int left = 0;
      SDL_GetWindowBordersSize(g_window.window, &top, &left, NULL, NULL);

      SDL_Log("Redefinindo dimensões da janela, de (%d, %d) para (%d, %d), e alterando a posição para (%d, %d).",
      DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, imageWidth, imageHeight, left, top);

      SDL_SetWindowSize(g_window.window, imageWidth, imageHeight);
      SDL_SetWindowPosition(g_window.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }

    SDL_SetWindowSize(g_windowChild.window, imageWidthChild, imageWidthHeight);

    int main_x, main_y, main_w, main_h;
    SDL_GetWindowPosition(g_window.window, &main_x, &main_y);
    SDL_GetWindowSize(g_window.window, &main_w, &main_h);
    int side_x = main_x + main_w + 10;
    int side_y = main_y;
    SDL_SetWindowPosition(g_windowChild.window, side_x, side_y);

    SDL_SyncWindow(g_window.window);
    SDL_SyncWindow(g_windowChild.window);
}

bool isGrayScale(SDL_Surface *surface)
{
  SDL_Log("<<< isGrayScale()");
  SDL_LockSurface(surface);
  Uint32 *pixel = (Uint32*)surface->pixels;
  int size = surface->w*surface->h;
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(surface->format);

  for(int i=0;i<size;i++)
  {
    Uint8 r,g,b,a;
    SDL_GetRGBA(pixel[i], format,NULL,&r,&g,&b,&a);
    if(!(r==g && g==b))
    {
      SDL_UnlockSurface(surface);
      return(false);
    }
  }

  SDL_Log("<<< isGrayScale()");
  SDL_UnlockSurface(surface);
  return(true);
}

void convertToGray(SDL_Surface *surface)
{
    SDL_Log("<<< convertToGray()");
    if (!surface) return;
    SDL_LockSurface(surface);
    Uint32 *pixel = (Uint32*)surface->pixels;
    int size = surface->w*surface->h;

    const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(surface->format);

    for (int i=0;i<size;i++)
    {
        Uint8 r,g,b,a;
        SDL_GetRGBA(pixel[i],format,NULL,&r,&g,&b,&a);
        double y = 0.2125*(double)r+0.7154*(double)g+0.0721*(double)b;
        Uint8 gray = (Uint8)y;

        pixel[i] = SDL_MapRGBA(format,NULL,gray,gray,gray,a);
    }

    SDL_UnlockSurface(surface);
    SDL_Log(">>> convertToGray()");
}

void loadImage(const char *filename, SDL_Renderer *renderer, MyImage *output_image)
{
  SDL_Log(">>> loadImage(\"%s\")", filename);

  if (!filename)
  {
    SDL_Log("\t*** Erro: Nome do arquivo inválido (filename == NULL).");
    SDL_Log("<<< loadImage(\"%s\")", filename);
    return;
  }

  if (!renderer)
  {
    SDL_Log("\t*** Erro: Renderer inválido (renderer == NULL).");
    SDL_Log("<<< loadImage(\"%s\")", filename);
    return;
  }

  if (!output_image)
  {
    SDL_Log("\t*** Erro: Imagem de saída inválida (output_image == NULL).");
    SDL_Log("<<< loadImage(\"%s\")", filename);
    return;
  }

  MyImage_destroy(output_image);

  SDL_Log("\tCarregando imagem \"%s\" em uma superfície...", filename);
  SDL_Surface *surface = IMG_Load(filename);
  if (!surface)
  {
    SDL_Log("\t*** Erro ao carregar a imagem: %s", SDL_GetError());
    SDL_Log("<<< loadImage(\"%s\")", filename);
    return;
  }

  SDL_Log("\tConvertendo superfície para formato RGBA32...");
  output_image->surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
  SDL_DestroySurface(surface);
  if (!output_image->surface)
  {
    SDL_Log("\t*** Erro ao converter superfície para formato RGBA32: %s", SDL_GetError());
    SDL_Log("<<< load_rgba32(\"%s\")", filename);
    return;
  }

  bool isGray = isGrayScale(output_image->surface);
  isGrayScale(output_image->surface) ? SDL_Log("\tÉ cinza") : SDL_Log("\tÉ colorida");

  if(!isGray){
    convertToGray(output_image->surface);
  }
  countIntensity(output_image->surface);
  originalSurface = SDL_ConvertSurface(output_image->surface, SDL_PIXELFORMAT_RGBA32);
  equalizedSurface = NULL;
  equalized = false;
  createTextureSurface(renderer);
  analyzeImage(output_image->surface);

  SDL_Log("<<< load_rgba32(\"%s\")", filename);
}

void createTextureSurface(SDL_Renderer *renderer)
{
  SDL_Surface *surface_to_use = equalized ? equalizedSurface : originalSurface;
  if (!surface_to_use) 
  {
    SDL_Log("*** Erro: Superfície para criar textura é NULL.");
    return;
  }

  if(g_image.texture) 
  {
    SDL_DestroyTexture(g_image.texture);
    g_image.texture = NULL;
  }

  g_image.texture = SDL_CreateTextureFromSurface(renderer, surface_to_use);
  if (!g_image.texture)
    SDL_Log("*** Erro ao criar textura: %s", SDL_GetError());

  SDL_Log("\tObtendo dimensões da textura...");
  SDL_GetTextureSize(g_image.texture, &g_image.rect.w, &g_image.rect.h);
}

void toggleButtonText()
{
    SDL_Log("<<< toggleButtonText()");
    if(strcmp(g_button.text, BUTTON_ORIGINAL) == 0)
      g_button.text = BUTTON_EQUALIZED;
    else
      g_button.text = BUTTON_ORIGINAL;
    if(g_button.text_texture)
    {
      SDL_DestroyTexture(g_button.text_texture);
      g_button.text_texture = NULL;
    }
    TTF_Font *font = TTF_OpenFont("font/Roboto-Regular.ttf", 15);
    if (!font) {
        SDL_Log("Erro ao carregar fonte: %s", SDL_GetError());
    }
    SDL_Surface *text_surface = TTF_RenderText_Blended(font, g_button.text, SDL_strlen(g_button.text), (SDL_Color){0,0,0,255});
    if(text_surface)
    {
      g_button.text_texture = SDL_CreateTextureFromSurface(g_windowChild.renderer, text_surface);
      g_button.text_w = (int)text_surface->w;
      g_button.text_h = (int)text_surface->h;
      SDL_DestroySurface(text_surface);
    }
    else
    {
      g_button.text_texture = NULL;
      g_button.text_w = g_button.text_h = 0;
      SDL_Log("*** Erro ao criar a superfície do texto: %s", SDL_GetError());
    }
    TTF_CloseFont(font);
    SDL_Log(">>> toggleButtonText()");
}

void renderButton()
{
  SDL_Log("<<< renderButton()");

  SDL_Color current_color = g_button.color_normal;
  if(g_button.is_pressed)
  {
      current_color = g_button.color_pressed;
  }else if(g_button.is_hovered)
  {
      current_color = g_button.color_hover;
  }
  if(g_button.text_texture)
  {
    int padding_x = 15;
    int padding_y = 5;
    int window_w, window_h;
    SDL_GetWindowSize(g_windowChild.window, &window_w, &window_h);
    SDL_FRect text_bg = {
      (window_w - (g_button.text_w + 2*padding_x)) / 2.0f,
      window_h - (g_button.text_h + 2*padding_y) - 5,
      g_button.text_w + 2*padding_x,
      g_button.text_h + 2*padding_y
    };
    g_button.rect = text_bg;
    SDL_SetRenderDrawColor(g_windowChild.renderer, current_color.r, current_color.g, current_color.b, current_color.a);
    SDL_RenderFillRect(g_windowChild.renderer, &text_bg);

    SDL_FRect text_rect = {
      text_bg.x + padding_x,
      text_bg.y + padding_y,
      g_button.text_w,
      g_button.text_h
    };
    SDL_RenderTexture(g_windowChild.renderer, g_button.text_texture, NULL, &text_rect);
  }
  SDL_Log(">>> renderButton()");
}

void createButton()
{
  SDL_Log("<<< createButton()");
  TTF_Font *font = TTF_OpenFont("font/Roboto-Regular.ttf", 15);
  if(!font)
  {
    SDL_Log("*** Erro ao abrir fonte: %s", SDL_GetError());
    return;
  }
  g_button.color_normal = (SDL_Color){0, 77, 156, 255}; //Azul
  g_button.color_hover = (SDL_Color){183, 219, 255, 255};  //Azul claro
  g_button.color_pressed = (SDL_Color){17, 59, 102, 255};   //Azul escuro
  
  g_button.text = BUTTON_ORIGINAL;
  
  SDL_Surface *text_surface = TTF_RenderText_Blended(font, BUTTON_ORIGINAL, SDL_strlen(BUTTON_ORIGINAL), (SDL_Color){0, 0, 0, 255});
  if(text_surface)
  {
    g_button.text_texture = SDL_CreateTextureFromSurface(g_windowChild.renderer, text_surface);
    g_button.text_w = (int)text_surface->w;
    g_button.text_h = (int)text_surface->h;
    SDL_DestroySurface(text_surface);
  }
  else
  {
    g_button.text_texture = NULL;
    g_button.text_w = 0;
    g_button.text_h = 0;
    SDL_Log("*** Erro ao criar a superfície do texto: %s", SDL_GetError());
  }
  TTF_CloseFont(font);
  SDL_Log(">>> createButton()");
}

void createHistogram()
{
  SDL_Log("<<< createHistogram()");
  g_hist.rect.w = 280;
  g_hist.rect.h = 200;

  g_hist.rect.x = (float)DEFAULT_WINDOW_CHILD_WIDTH / 2.0f - (float)g_hist.rect.w / 2.0f;

  g_hist.rect.y = (float)DEFAULT_WINDOW_CHILD_HEIGHT / 2.0f - (float)g_hist.rect.h/ 1.7f; 
  SDL_Log(">>> createHistogram()");
}

void renderHistogramBars()
{
  SDL_Log("<<< renderHistogramBars()");
  float *intensity = equalized ? counterIntensityEqualized : counterIntensity;

  SDL_SetRenderDrawColor(g_windowChild.renderer, 0, 0, 0, 255);

  float base_y = g_hist.rect.y + g_hist.rect.h - 1;
  float max_bar_height = g_hist.rect.h - 20;
  float bar_w = g_hist.rect.w / 256.0f;

  float max_value = 0.0f;
  for(int i=0;i<256;i++)
      if(intensity[i] > max_value) max_value = intensity[i];
  if(max_value == 0) max_value = 1.0f;

  for(int i=0;i<256;i++)
  {
      SDL_FRect bar;
      bar.w = bar_w;
      bar.h = (intensity[i] / max_value) * max_bar_height;
      bar.x = g_hist.rect.x + i * bar_w;
      bar.y = base_y - bar.h;
      histBars[i] = bar;
      SDL_RenderFillRect(g_windowChild.renderer, &bar);
  }

  SDL_SetRenderDrawColor(g_windowChild.renderer, 255, 0, 239, 255);
  SDL_RenderLine(g_windowChild.renderer,
                  g_hist.rect.x,
                  base_y,
                  g_hist.rect.x + g_hist.rect.w,
                  base_y);

  SDL_RenderLine(g_windowChild.renderer,
                  g_hist.rect.x,
                  g_hist.rect.y,
                  g_hist.rect.x,
                  base_y);
  SDL_Log(">>> renderHistogramBars()");
}

void equalize(SDL_Surface *surface)
{
  SDL_Log("<<< equalize()");
  if (!surface) return;

  SDL_LockSurface(surface);

  Uint32 *pixel = (Uint32*)surface->pixels;

  int size = surface->w * surface->h;

  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(surface->format);

  int hist[256] = {0};

  for(int i=0;i<size;i++) 
  {
    Uint8 r,g,b,a;
    SDL_GetRGBA(pixel[i], format, NULL, &r, &g, &b, &a);
    hist[r]++; 
  }

  int cdf[256];

  cdf[0] = hist[0];

  for(int i = 1; i < 256; i++) 
  {
    cdf[i] = cdf[i-1] + hist[i];
  }

  Uint8 lut[256];

  for(int i = 0; i < 256; i++) 
  {
    lut[i] = (Uint8)((cdf[i] * 255.0) / size + 0.05);
  }

  for(int i = 0; i < size; i++) 
  {
    Uint8 r,g,b,a;
    SDL_GetRGBA(pixel[i], format, NULL, &r, &g, &b, &a);
    Uint8 eq = lut[r];
    pixel[i] = SDL_MapRGBA(format, NULL, eq, eq, eq, a);
  }

  SDL_UnlockSurface(surface);

  for(int i = 0; i < 256; i++)
    counterIntensityEqualized[i] = 0.0f;

  for(int i = 0; i < size; i++) 
  {
    Uint8 r,g,b,a;
    SDL_GetRGBA(pixel[i], format, NULL, &r, &g, &b, &a);
    counterIntensityEqualized[r] += 1.0f;
  }

  equalizedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);

  SDL_Log(">>> equalize()");
}

void loadHistogramButton()
{
  createButton();
  createHistogram();
}

void countIntensity(SDL_Surface *surface)
{
  SDL_Log(">>> countIntensity()");

  if (!surface) return;

  for(int i = 0; i < 256; i++)
  {
      counterIntensity[i] = 0.0f;
      counterIntensityEqualized[i] = 0.0f;
  }

  SDL_LockSurface(surface);
  Uint32 *pixels = (Uint32*)surface->pixels;
  int size = surface->w * surface->h;
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(surface->format);

  for(int i=0;i<size;i++)
  {
      Uint8 r, g, b, a;
      SDL_GetRGBA(pixels[i], format, NULL, &r, &g, &b, &a);
      Uint8 intensity = r;

      if(equalized)
          counterIntensityEqualized[intensity]++;
      else
          counterIntensity[intensity]++;
  }

  SDL_UnlockSurface(surface);

  for(int i=0;i<256;i++)
  {
    if(equalized)
        counterIntensityEqualized[i] = (counterIntensityEqualized[i] / size) * 100.0f;
    else
        counterIntensity[i] = (counterIntensity[i] / size) * 100.0f;
  }

  SDL_Log("<<< countIntensity()");
}

void analyzeImage(SDL_Surface *surface) 
{
  SDL_Log(">>> analyzeImage()");
  if (!surface) return;

  SDL_LockSurface(surface);
  Uint32 *pixels = (Uint32*)surface->pixels;
  int size = surface->w * surface->h;
  const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(surface->format);

  double sum = 0.0;
  double sum_sq = 0.0;

  for (int i = 0; i < size; i++) 
  {
    Uint8 r, g, b, a;
    SDL_GetRGBA(pixels[i], format, NULL, &r, &g, &b, &a);
    Uint8 intensity = r; 
    sum += intensity;
    sum_sq += intensity * intensity;
  }

  SDL_UnlockSurface(surface);

  double mean = sum / size;
  double variance = (sum_sq / size) - (mean * mean);
  double stddev = sqrt(variance);

  if (mean < 85) brightness = "Imagem: Escura";
  else if (mean < 170) brightness = "Imagem: Média";
  else brightness = "Imagem: Clara";

  if (stddev < 50) contrast = "Contraste: Baixo";
  else if (stddev < 100) contrast = "Contraste: Médio";
  else contrast = "Contraste: Alto";

  SDL_Log("Média: %.2f (%s)", mean, brightness);
  SDL_Log("Desvio padrão: %.2f (Contraste %s)", stddev, contrast);
}

void renderImageStats() 
{
  TTF_Font *font = TTF_OpenFont("font/Roboto-Regular.ttf", 14);
  if (!font) 
  {
    SDL_Log("Erro ao carregar fonte: %s", SDL_GetError());
    return;
  }

  SDL_Surface *text_surface1 = TTF_RenderText_Blended(font, brightness, SDL_strlen(brightness), (SDL_Color){0,0,0,255});
  SDL_Texture *text_texture1 = SDL_CreateTextureFromSurface(g_windowChild.renderer, text_surface1);

  SDL_Surface *text_surface2 = TTF_RenderText_Blended(font, contrast, SDL_strlen(contrast), (SDL_Color){0,0,0,255});
  SDL_Texture *text_texture2 = SDL_CreateTextureFromSurface(g_windowChild.renderer, text_surface2);

  int win_w, win_h;
  SDL_GetWindowSize(g_windowChild.window, &win_w, &win_h);

  SDL_FRect text_rect1 = { (win_w - text_surface1->w) / 2.0f, 10, text_surface1->w, text_surface1->h };
  SDL_RenderTexture(g_windowChild.renderer, text_texture1, NULL, &text_rect1);

  SDL_FRect text_rect2 = { (win_w - text_surface2->w) / 2.0f, 10 + text_surface1->h + 5, text_surface2->w, text_surface2->h };
  SDL_RenderTexture(g_windowChild.renderer, text_texture2, NULL, &text_rect2);

  SDL_DestroySurface(text_surface1);
  SDL_DestroySurface(text_surface2);
  SDL_DestroyTexture(text_texture1);
  SDL_DestroyTexture(text_texture2);

  TTF_CloseFont(font);
}

//------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    atexit(shutdown);

    if(argc < 2) 
    {
        SDL_Log("Uso: %s <arquivo de imagem>", argv[0]);
        return SDL_APP_FAILURE;
    }

    if (initialize() == SDL_APP_FAILURE)
        return SDL_APP_FAILURE;

    IMAGE_FILENAME = argv[1];

    loadImage(IMAGE_FILENAME, g_window.renderer, &g_image);

    loadHistogramButton();

    createWindow();

    loop();

    return 0;
}