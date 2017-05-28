#include <string>
#include <cstring>
#include <stdio.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include "PixelBone/gfx.hpp"
#include "PixelBone/matrix.hpp"

#include "fastmemcpy.h"

#include <linux/fb.h>

const float BOOST_PSI_MAX = 21.0;
const int IAT_HOT_THRESHOLD = 150;
const int IAT_COLD_THRESHOLD = 32;
const int KNOCK_PROBLEM_THRESHOLD = 5;

struct boost_readings_t {
  float boost_psi_current;
  float boost_psi_max;
  int iat;
  int knock;
};

class LedRingDisplay {
  boost_readings_t *active_boost_readings;
  std::thread render_thread;

public:
  LedRingDisplay(boost_readings_t &boost_readings) {
    active_boost_readings = &boost_readings;
  }

  void initialize() {
  }

  void start() {
    render_thread = std::thread([&]() {
      while(1) {
        this->render_led_ring();
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
      }
    });
  }

  void cleanup() {
  }

private:

  uint32_t led_red = PixelBone_Pixel::Color(211/12, 0, 153/24);
  uint32_t led_blue = PixelBone_Pixel::Color(97/24, 169/24, 255/12);
  uint32_t led_green = PixelBone_Pixel::Color(54/24, 227/12, 132/24);
  // uint32_t led_red = PixelBone_Pixel::Color(211, 0, 153/2);
  // uint32_t led_blue = PixelBone_Pixel::Color(97/4, 169/4, 255);
  // uint32_t led_green = PixelBone_Pixel::Color(54/4, 227, 132/4);

  PixelBone_Matrix matrix = PixelBone_Matrix(24, 1, MATRIX_TOP + MATRIX_LEFT + MATRIX_ROWS + MATRIX_ZIGZAG);

  void render_led_ring() {
    //TODO Use reference as it lies. Cheating to make refactor faster.
    float boost = active_boost_readings->boost_psi_current;
    matrix.fillScreen(0);

    uint32_t color = active_boost_readings->boost_psi_current >= BOOST_PSI_MAX ? led_red : led_green;
    if (active_boost_readings->boost_psi_current > 0) {
      uint32_t color = active_boost_readings->boost_psi_current >= BOOST_PSI_MAX ? led_red : led_green;
      for (int i=0; i<=active_boost_readings->boost_psi_current; i++) {
        matrix.drawPixel(i, 0, color);
      }
    } else {
      for (int i=24; i>=active_boost_readings->boost_psi_current+24; i--) {
        matrix.drawPixel(i, 0, led_blue);
      }
    }
    matrix.show();
  }

};

class LcdDisplay
{
  const std::string BOOST_LABEL = "BOOST (PSI)";
  const std::string IAT_LABEL = "IAT";
  const std::string PMAX_LABEL = "P.MAX";
  const std::string KNOCK_LABEL = "K";
  const int BOOST_X_CENTER = 64;
  const int IAT_X_CENTER = 16;
  const int PMAX_X_CENTER = 64;
  const int KNOCK_X_CENTER = 115;

  char boost_psi_current_formatted [7];
  char boost_psi_max_formatted [6];
  char iat_formatted [5];
  char knock_formatted [4];
  char last_boost_psi_current_formatted [7];
  char last_boost_psi_max_formatted [6];
  char last_iat_formatted [5];
  char last_knock_formatted [4];

  Cairo::TextExtents extents;
  const Cairo::RefPtr<Cairo::SolidPattern> red_color = Cairo::SolidPattern::create_rgb(0.83, 0.0, 0.6);
  const Cairo::RefPtr<Cairo::SolidPattern> green_color = Cairo::SolidPattern::create_rgb(0.14, 0.9, 0.5);
  const Cairo::RefPtr<Cairo::SolidPattern> blue_color = Cairo::SolidPattern::create_rgb(0.38, 0.66, 1.0);
  const Cairo::RefPtr<Cairo::SolidPattern> white_color = Cairo::SolidPattern::create_rgb(1.0, 1.0, 1.0);
  const Cairo::RefPtr<Cairo::SolidPattern> gray_color = Cairo::SolidPattern::create_rgb(0.6, 0.6, 0.6);
  const Cairo::RefPtr<Cairo::SolidPattern> black_color = Cairo::SolidPattern::create_rgb(0.0, 0.0, 0.0);

  const Cairo::RefPtr<Cairo::ToyFontFace> number_font =
    Cairo::ToyFontFace::create("Eurostile",
                                Cairo::FONT_SLANT_NORMAL,
                                Cairo::FONT_WEIGHT_BOLD);
  const Cairo::RefPtr<Cairo::ToyFontFace> label_font =
    Cairo::ToyFontFace::create("Avenir Next",
                                Cairo::FONT_SLANT_NORMAL,
                                Cairo::FONT_WEIGHT_BOLD);

  struct fb_var_screeninfo screen_info;
  struct fb_fix_screeninfo fixed_info;
  unsigned char *front_buffer = NULL;
  unsigned char *back_buffer = NULL;
  size_t buflen;
  int fd = -1;
  Cairo::RefPtr<Cairo::ImageSurface> cairo_surface;
  Cairo::RefPtr<Cairo::Context> cairo_context;
  std::thread render_thread;
  boost_readings_t *active_boost_readings;

public:
  LcdDisplay(boost_readings_t &boost_readings) {
    active_boost_readings = &boost_readings;
  }

  void initialize() {
    fd = open("/dev/fb0", O_RDWR);
    if (fd == 0) {
      perror("open");
      cleanup();
      exit(1);
    }
    if (ioctl(fd, FBIOGET_VSCREENINFO, &screen_info) || ioctl(fd, FBIOGET_FSCREENINFO, &fixed_info)) {
      perror("ioctl");
      cleanup();
      exit(1);
    }

    buflen = screen_info.yres_virtual * fixed_info.line_length;
    front_buffer = (unsigned char *) mmap(NULL, buflen, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    back_buffer = new unsigned char[buflen];

    if (front_buffer == MAP_FAILED) {
      perror("mmap");
      cleanup();
      exit(1);
    }
    cairo_surface = Cairo::ImageSurface::create(back_buffer, Cairo::FORMAT_RGB16_565, 128, 128, Cairo::ImageSurface::format_stride_for_width(Cairo::FORMAT_RGB16_565, screen_info.xres_virtual));

    cairo_context = Cairo::Context::create(cairo_surface);
  }

  void cleanup() {
    if (front_buffer && front_buffer != MAP_FAILED)
      munmap(front_buffer, buflen);
    if (fd >= 0)
      close(fd);
  }

  void start() {
    this->draw_gauge_background();
    render_thread = std::thread([&]() {
      while(1) {
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        this->render_lcd();
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
        std::cout << "It took me " << time_span.count() << " seconds.\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(40));
      }
    });
  }

private:

  void draw_gauge_background() {
    cairo_context->save();
    cairo_context->paint();

    // Draw labels
    cairo_context->set_source(gray_color);
    cairo_context->set_font_face(label_font);
    cairo_context->set_font_size(12.0);

    // BOOST label
    cairo_context->get_text_extents(BOOST_LABEL, extents);
    cairo_context->move_to(BOOST_X_CENTER-(extents.width/2 + extents.x_bearing), 11);
    cairo_context->show_text(BOOST_LABEL);

    // IAT label
    cairo_context->get_text_extents(IAT_LABEL, extents);
    cairo_context->move_to(IAT_X_CENTER-(extents.width/2 + extents.x_bearing), 110);
    cairo_context->show_text(IAT_LABEL);

    // P.MAX label
    cairo_context->get_text_extents(PMAX_LABEL, extents);
    cairo_context->move_to(PMAX_X_CENTER-(extents.width/2 + extents.x_bearing), 110);
    cairo_context->show_text(PMAX_LABEL);

    // Knock label
    cairo_context->get_text_extents(KNOCK_LABEL, extents);
    cairo_context->move_to(KNOCK_X_CENTER-(extents.width/2 + extents.x_bearing), 110);
    cairo_context->show_text(KNOCK_LABEL);

    cairo_context->restore();
  }


  void draw_numbers() {
    //TODO Use the references as they are. Cheating so simplify refactor.
    float boost_psi_max = active_boost_readings->boost_psi_max;
    int iat = active_boost_readings->iat;
    int knock = active_boost_readings->knock;

    cairo_context->save();

    cairo_context->set_font_face(number_font);

    // Boost
    snprintf(boost_psi_current_formatted, 7, "% 3.1f", active_boost_readings->boost_psi_current);
    if (std::strcmp(boost_psi_current_formatted, last_boost_psi_current_formatted) != 0) {
      strncpy(last_boost_psi_current_formatted, boost_psi_current_formatted, 7);
      cairo_context->set_source(black_color);
      cairo_context->rectangle(0, 14, 128, 32);
      cairo_context->fill();

      if (active_boost_readings->boost_psi_current < 0)
        cairo_context->set_source(blue_color);
      else if (active_boost_readings->boost_psi_current < BOOST_PSI_MAX)
        cairo_context->set_source(green_color);
      else
        cairo_context->set_source(red_color);
      cairo_context->set_font_size(45.0);
      cairo_context->get_text_extents(boost_psi_current_formatted, extents);
      cairo_context->move_to(BOOST_X_CENTER-(extents.width/2 + extents.x_bearing), 45);
      cairo_context->show_text(boost_psi_current_formatted);
    }

    cairo_context->set_font_size(16.0);

    // IAT
    snprintf(iat_formatted, 5, "% 3d", iat);
    if (std::strcmp(iat_formatted, last_iat_formatted) != 0) {
      strncpy(last_iat_formatted, iat_formatted, 5);
      cairo_context->set_source(black_color);
      cairo_context->rectangle(IAT_X_CENTER - 16, 112, 32, 16);
      cairo_context->fill();

      if (iat > IAT_HOT_THRESHOLD)
        cairo_context->set_source(red_color);
      else if (iat < IAT_COLD_THRESHOLD)
        cairo_context->set_source(blue_color);
      else
        cairo_context->set_source(green_color);
      cairo_context->get_text_extents(iat_formatted, extents);
      cairo_context->move_to(IAT_X_CENTER-(extents.width/2 + extents.x_bearing), 126);
      cairo_context->show_text(iat_formatted);
    }

    // P. MAX
    snprintf(boost_psi_max_formatted, 6, "% 2.1f", boost_psi_max);
    if (std::strcmp(boost_psi_max_formatted, last_boost_psi_max_formatted) != 0) {
      strncpy(last_boost_psi_max_formatted, boost_psi_max_formatted, 6);
      cairo_context->set_source(black_color);
      cairo_context->rectangle(PMAX_X_CENTER-25, 112, 50, 16);
      cairo_context->fill();
      if (boost_psi_max < 0)
        cairo_context->set_source(blue_color);
      else if (boost_psi_max < BOOST_PSI_MAX)
        cairo_context->set_source(green_color);
      else
        cairo_context->set_source(red_color);
      cairo_context->get_text_extents(boost_psi_max_formatted, extents);
      cairo_context->move_to(PMAX_X_CENTER-(extents.width/2 + extents.x_bearing), 126);
      cairo_context->show_text(boost_psi_max_formatted);
    }

    // Knock
    snprintf(knock_formatted, 4, "%2d", knock);
    if (std::strcmp(knock_formatted, last_knock_formatted) != 0) {
      strncpy(last_knock_formatted, knock_formatted, 4);
      cairo_context->set_source(black_color);
      cairo_context->rectangle(KNOCK_X_CENTER-15, 112, 30, 16);
      cairo_context->fill();
      if (knock > KNOCK_PROBLEM_THRESHOLD)
        cairo_context->set_source(red_color);
      else
        cairo_context->set_source(green_color);
      cairo_context->get_text_extents(knock_formatted, extents);
      cairo_context->move_to(KNOCK_X_CENTER-(extents.width/2 + extents.x_bearing), 126);
      cairo_context->show_text(knock_formatted);
    }

    cairo_context->restore();
  }

  float *boosts = new float[127];
  int boosts_current_index = -1;
  const int GRAPH_BOOST_HEIGHT = 25;
  const int GRAPH_VACUUM_HEIGHT = 25;
  const int GRAPH_V_CENTER = 73;
  const float GRAPH_BOOST_FACTOR = (25.0 / 22.0);
  const float GRAPH_VACUUM_FACTOR = (25.0 / 35.0);
  float corrected_graph_pressure = 0;

  void draw_graph() {
    boosts_current_index = (boosts_current_index + 1) % 128;
    boosts[boosts_current_index] = active_boost_readings->boost_psi_current;

    cairo_context->save();
    cairo_context->set_antialias(Cairo::Antialias::ANTIALIAS_NONE);
    cairo_context->set_line_width(1.0);

    cairo_context->set_source(black_color);
    cairo_context->rectangle(0, GRAPH_V_CENTER - GRAPH_VACUUM_HEIGHT, 128, GRAPH_BOOST_HEIGHT + GRAPH_VACUUM_HEIGHT);
    cairo_context->fill();

    for (int i=0; i<128; i++) {
      int corrected_index = (i + boosts_current_index) % 128;
      if (boosts[corrected_index] <= 0) {
        cairo_context->set_source(blue_color);
        corrected_graph_pressure = boosts[corrected_index] * GRAPH_VACUUM_FACTOR;
      } else if (boosts[corrected_index] >= BOOST_PSI_MAX) {
        cairo_context->set_source(red_color);
        corrected_graph_pressure = boosts[corrected_index] * GRAPH_BOOST_FACTOR;
      } else {
        cairo_context->set_source(green_color);
        corrected_graph_pressure = boosts[corrected_index] * GRAPH_BOOST_FACTOR;
      }
      cairo_context->move_to(i, 72);
      cairo_context->line_to(i, 72 - (int) corrected_graph_pressure);
      cairo_context->stroke();
    }

    cairo_context->set_source(white_color);
    cairo_context->move_to(0, 73);
    cairo_context->line_to(128, 73);
    cairo_context->stroke();
    cairo_context->restore();
  }

  void render_lcd() {
    draw_numbers();
    draw_graph();
    memcpy(front_buffer, back_buffer, buflen);
  }

};

int main()
{

  boost_readings_t readings = {0, 0, 70, 0};

  // Simulation stuff
  float boost_psi_step = 0.01;
  int iat_count_interval = 25;
  int iat_step = 1;
  int knock_count_interval = 100;
  int knock_step = 1;
  int loop_counter = 0;

  // Display Setup
  LcdDisplay lcd_display(readings);
  lcd_display.initialize();
  lcd_display.start();

  LedRingDisplay led_ring_display(readings);
  led_ring_display.initialize();
  led_ring_display.start();

  // Simulation Loop
  while (1) {
    readings.boost_psi_current += boost_psi_step;
    if (readings.boost_psi_current > 21.8 || readings.boost_psi_current < -32.0)
      boost_psi_step = boost_psi_step * -1;

    if (loop_counter % iat_count_interval == 0) {
      readings.iat += iat_step;
      if (readings.iat > 250 || readings.iat < -20)
        iat_step = iat_step * -1;
    }
    if (loop_counter % knock_count_interval == 0) {
      readings.knock += knock_step;
      if (readings.knock > 98 || readings.knock < 1)
        knock_step = knock_step * -1;
    }

    if (readings.boost_psi_max < readings.boost_psi_current) 
      readings.boost_psi_max = readings.boost_psi_current;

    usleep(1000);
    loop_counter++;
  }

  lcd_display.cleanup();
  led_ring_display.cleanup();

  return 0;
}
