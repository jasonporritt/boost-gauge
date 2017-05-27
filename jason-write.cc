#include <string>
#include <cstring>
#include <stdio.h>
#include <chrono>
#include <iostream>
#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>
#include <cairomm/quartz_surface.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include "PixelBone/gfx.hpp"
#include "PixelBone/matrix.hpp"

#include <linux/fb.h>

const float BOOST_PSI_MAX = 21.0;
const int IAT_HOT_THRESHOLD = 150;
const int IAT_COLD_THRESHOLD = 32;
const int KNOCK_PROBLEM_THRESHOLD = 5;

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

void draw_gauge_background(Cairo::RefPtr<Cairo::Context> cr) {
  cr->save();
  cr->paint();

  // Draw labels
  cr->set_source(gray_color);
  cr->set_font_face(label_font);
  cr->set_font_size(12.0);

  // BOOST label
  cr->get_text_extents(BOOST_LABEL, extents);
  cr->move_to(BOOST_X_CENTER-(extents.width/2 + extents.x_bearing), 11);
  cr->show_text(BOOST_LABEL);

  // IAT label
  cr->get_text_extents(IAT_LABEL, extents);
  cr->move_to(IAT_X_CENTER-(extents.width/2 + extents.x_bearing), 110);
  cr->show_text(IAT_LABEL);

  // P.MAX label
  cr->get_text_extents(PMAX_LABEL, extents);
  cr->move_to(PMAX_X_CENTER-(extents.width/2 + extents.x_bearing), 110);
  cr->show_text(PMAX_LABEL);

  // Knock label
  cr->get_text_extents(KNOCK_LABEL, extents);
  cr->move_to(KNOCK_X_CENTER-(extents.width/2 + extents.x_bearing), 110);
  cr->show_text(KNOCK_LABEL);

  cr->restore();
}


void draw_numbers(Cairo::RefPtr<Cairo::Context> cr, float boost_psi_current, float boost_psi_max, int iat, int knock) {
  cr->save();

  cr->set_font_face(number_font);

  // Boost
  snprintf(boost_psi_current_formatted, 7, "% 3.1f", boost_psi_current);
  if (std::strcmp(boost_psi_current_formatted, last_boost_psi_current_formatted) != 0) {
    strncpy(last_boost_psi_current_formatted, boost_psi_current_formatted, 7);
    cr->set_source(black_color);
    cr->rectangle(0, 14, 128, 32);
    cr->fill();

    if (boost_psi_current < 0)
      cr->set_source(blue_color);
    else if (boost_psi_current < BOOST_PSI_MAX)
      cr->set_source(green_color);
    else
      cr->set_source(red_color);
    cr->set_font_size(45.0);
    cr->get_text_extents(boost_psi_current_formatted, extents);
    cr->move_to(BOOST_X_CENTER-(extents.width/2 + extents.x_bearing), 45);
    cr->show_text(boost_psi_current_formatted);
  }

  cr->set_font_size(16.0);

  // IAT
  snprintf(iat_formatted, 5, "% 3d", iat);
  if (std::strcmp(iat_formatted, last_iat_formatted) != 0) {
    strncpy(last_iat_formatted, iat_formatted, 5);
    cr->set_source(black_color);
    cr->rectangle(IAT_X_CENTER - 16, 112, 32, 16);
    cr->fill();

    if (iat > IAT_HOT_THRESHOLD)
      cr->set_source(red_color);
    else if (iat < IAT_COLD_THRESHOLD)
      cr->set_source(blue_color);
    else
      cr->set_source(green_color);
    cr->get_text_extents(iat_formatted, extents);
    cr->move_to(IAT_X_CENTER-(extents.width/2 + extents.x_bearing), 126);
    cr->show_text(iat_formatted);
  }

  // P. MAX
  snprintf(boost_psi_max_formatted, 6, "% 2.1f", boost_psi_max);
  if (std::strcmp(boost_psi_max_formatted, last_boost_psi_max_formatted) != 0) {
    strncpy(last_boost_psi_max_formatted, boost_psi_max_formatted, 6);
    cr->set_source(black_color);
    cr->rectangle(PMAX_X_CENTER-25, 112, 50, 16);
    cr->fill();
    if (boost_psi_max < 0)
      cr->set_source(blue_color);
    else if (boost_psi_max < BOOST_PSI_MAX)
      cr->set_source(green_color);
    else
      cr->set_source(red_color);
    cr->get_text_extents(boost_psi_max_formatted, extents);
    cr->move_to(PMAX_X_CENTER-(extents.width/2 + extents.x_bearing), 126);
    cr->show_text(boost_psi_max_formatted);
  }

  // Knock
  snprintf(knock_formatted, 4, "%2d", knock);
  if (std::strcmp(knock_formatted, last_knock_formatted) != 0) {
    strncpy(last_knock_formatted, knock_formatted, 4);
    cr->set_source(black_color);
    cr->rectangle(KNOCK_X_CENTER-15, 112, 30, 16);
    cr->fill();
    if (knock > KNOCK_PROBLEM_THRESHOLD)
      cr->set_source(red_color);
    else
      cr->set_source(green_color);
    cr->get_text_extents(knock_formatted, extents);
    cr->move_to(KNOCK_X_CENTER-(extents.width/2 + extents.x_bearing), 126);
    cr->show_text(knock_formatted);
  }

  cr->restore();
}

float *boosts = new float[127];
int boosts_current_index = -1;
const int GRAPH_BOOST_HEIGHT = 25;
const int GRAPH_VACUUM_HEIGHT = 25;
const int GRAPH_V_CENTER = 73;
const float GRAPH_BOOST_FACTOR = (25.0 / 22.0);
const float GRAPH_VACUUM_FACTOR = (25.0 / 35.0);
float corrected_graph_pressure = 0;
void draw_graph(Cairo::RefPtr<Cairo::Context> cr, float boost_psi_current) {
  boosts_current_index = (boosts_current_index + 1) % 128;
  boosts[boosts_current_index] = boost_psi_current;

  cr->save();
  cr->set_antialias(Cairo::Antialias::ANTIALIAS_NONE);
  cr->set_line_width(1.0);

  cr->set_source(black_color);
  cr->rectangle(0, GRAPH_V_CENTER - GRAPH_VACUUM_HEIGHT, 128, GRAPH_BOOST_HEIGHT + GRAPH_VACUUM_HEIGHT);
  cr->fill();

  for (int i=0; i<128; i++) {
    int corrected_index = (i + boosts_current_index) % 128;
    if (boosts[corrected_index] <= 0) {
      cr->set_source(blue_color);
      corrected_graph_pressure = boosts[corrected_index] * GRAPH_VACUUM_FACTOR;
    } else if (boosts[corrected_index] >= BOOST_PSI_MAX) {
      cr->set_source(red_color);
      corrected_graph_pressure = boosts[corrected_index] * GRAPH_BOOST_FACTOR;
    } else {
      cr->set_source(green_color);
      corrected_graph_pressure = boosts[corrected_index] * GRAPH_BOOST_FACTOR;
    }
    cr->move_to(i, 72);
    cr->line_to(i, 72 - (int) corrected_graph_pressure);
    cr->stroke();
  }

  cr->set_source(white_color);
  cr->move_to(0, 73);
  cr->line_to(128, 73);
  cr->stroke();
  cr->restore();
}

Cairo::RefPtr<Cairo::Context> setup(Cairo::RefPtr<Cairo::Surface> surface) {
  Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);
  draw_gauge_background(cr);
  return cr;
}
void render(Cairo::RefPtr<Cairo::Context> cr, float boost_psi_current, float boost_psi_max, int iat, int knock) {
  cr->save(); // save the state of the context
  draw_numbers(cr, boost_psi_current, boost_psi_max, iat, knock);
  draw_graph(cr, boost_psi_current);
}

uint32_t led_red = PixelBone_Pixel::Color(211/5, 0, 153/10);
uint32_t led_dim_red = PixelBone_Pixel::Color(211/20, 0, 153/40);
uint32_t led_blue = PixelBone_Pixel::Color(0, 169/10, 255/5);
uint32_t led_dim_blue = PixelBone_Pixel::Color(0, 169/40, 255/20);
uint32_t led_green = PixelBone_Pixel::Color(54/10, 227/5, 132/10);
uint32_t led_dim_green = PixelBone_Pixel::Color(54/40, 227/20, 132/40);
// uint32_t led_red = PixelBone_Pixel::Color(211, 0, 153/2);
// uint32_t led_blue = PixelBone_Pixel::Color(97/4, 169/4, 255);
// uint32_t led_green = PixelBone_Pixel::Color(54/4, 227, 132/4);

PixelBone_Matrix matrix(24,1,
  MATRIX_TOP  + MATRIX_LEFT +
  MATRIX_ROWS + MATRIX_ZIGZAG);

void render_led_ring(float boost) {
  matrix.fillScreen(0);
  uint32_t color = boost >= BOOST_PSI_MAX ? led_red : led_green;
  uint32_t dim_color = boost >= BOOST_PSI_MAX ? led_dim_red : led_dim_green;
  if (boost > 0) {
    for (int i=0; i<=boost+1; i++) {
      if (i-boost > 0 && i-boost < 0.5) {
      	matrix.drawPixel((i+10) % 24, 0, dim_color);
      }
      else if (i-boost > 0) {
        // nothing
      }
      else {
        matrix.drawPixel((i+10) % 24, 0, color);
      }
    }
  } else {
    for (int i=23+10; i>=boost+23+10; i--) {
      matrix.drawPixel(i % 24, 0, led_blue);
    }
  }
  matrix.show();
}

int main()
{
  float boost_psi = 0;
  float boost_psi_max = 0;
  int iat = 70;
  int knock = 0;

  // Simulation stuff
  float boost_psi_step = 0.001;
  int iat_count_interval = 25;
  int iat_step = 1;
  int knock_count_interval = 100;
  int knock_step = 1;
  int loop_counter = 0;

  struct fb_var_screeninfo screen_info;
  struct fb_fix_screeninfo fixed_info;
  unsigned char *front_buffer = NULL;
  unsigned char *back_buffer = NULL;
  size_t buflen;
  int fd = -1;
  int r = 1;

  fd = open("/dev/fb0", O_RDWR);
  if (fd >= 0)
  {
    if (!ioctl(fd, FBIOGET_VSCREENINFO, &screen_info) &&
        !ioctl(fd, FBIOGET_FSCREENINFO, &fixed_info))
    {
        buflen = screen_info.yres_virtual * fixed_info.line_length;
        front_buffer = (unsigned char *) mmap(NULL,
                      buflen,
                      PROT_READ|PROT_WRITE,
                      MAP_SHARED,
                      fd,
                      0);
        back_buffer = new unsigned char[buflen];

        if (front_buffer != MAP_FAILED)
        {
            Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(back_buffer, Cairo::FORMAT_RGB16_565, 128, 128, Cairo::ImageSurface::format_stride_for_width(Cairo::FORMAT_RGB16_565, screen_info.xres_virtual));

            Cairo::RefPtr<Cairo::Context> context = setup(surface);

            struct fb_var_screeninfo vinfo;

            auto last_time = std::chrono::steady_clock::now();
            auto current_time = std::chrono::steady_clock::now();
            double ns_since_last_render = 0;
            while (1) {
              current_time = std::chrono::steady_clock::now();
              ns_since_last_render += std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - last_time).count();
              last_time = current_time;

              if (ns_since_last_render >= 33333333) {
                ns_since_last_render = ((int)ns_since_last_render) % 33333333;
                render_led_ring(boost_psi);
                render(context, boost_psi, boost_psi_max, iat, knock);
                memcpy(front_buffer, back_buffer, buflen);
              }

              boost_psi += boost_psi_step;
              if (boost_psi > 21.8 || boost_psi < -32.0)
                boost_psi_step = boost_psi_step * -1;

              if (loop_counter % iat_count_interval == 0) {
                iat += iat_step;
                if (iat > 250 || iat < -20)
                  iat_step = iat_step * -1;
              }
              if (loop_counter % knock_count_interval == 0) {
                knock += knock_step;
                if (knock > 98 || knock < 1)
                  knock_step = knock_step * -1;
              }

              if (boost_psi_max < boost_psi) 
                boost_psi_max = boost_psi;

              usleep(100);
              loop_counter++;
            }

            r = 0;   /* Indicate success */
        }
        else
        {
          perror("mmap");
        }
    }
    else
    {
        perror("ioctl");
    }
  }
  else
  {
    perror("open");
  }

  /*
  * Clean up
  */
  if (front_buffer && front_buffer != MAP_FAILED)
    munmap(front_buffer, buflen);
  if (fd >= 0)
    close(fd);

  return r;
}
