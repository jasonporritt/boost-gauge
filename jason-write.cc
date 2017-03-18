#include <string>
#include <stdio.h>
#include <iostream>
#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef __linux__
#include <linux/fb.h>
#else
#endif

const float BOOST_PSI_MAX = 21.0;
const int IAT_HOT_THRESHOLD = 150;
const int IAT_COLD_THRESHOLD = 32;
const int KNOCK_PROBLEM_THRESHOLD = 5;

const std::string BOOST_LABEL = "BOOST (PSI)";
const std::string IAT_LABEL = "IAT";
const std::string PMAX_LABEL = "P.MAX";
const std::string KNOCK_LABEL = "K";
const int BOOST_X_CENTER = 64;
const int IAT_X_CENTER = 20;
const int PMAX_X_CENTER = 70;
const int KNOCK_X_CENTER = 118;

char boost_psi_current_formatted [7];
char boost_psi_max_formatted [6];
char iat_formatted [5];
char knock_formatted [4];

Cairo::TextExtents extents;
const Cairo::RefPtr<Cairo::SolidPattern> red_color = Cairo::SolidPattern::create_rgb(0.83, 0.0, 0.6);
const Cairo::RefPtr<Cairo::SolidPattern> green_color = Cairo::SolidPattern::create_rgb(0.14, 0.9, 0.5);
const Cairo::RefPtr<Cairo::SolidPattern> blue_color = Cairo::SolidPattern::create_rgb(0.38, 0.66, 1.0);
const Cairo::RefPtr<Cairo::SolidPattern> white_color = Cairo::SolidPattern::create_rgb(1.0, 1.0, 1.0);
const Cairo::RefPtr<Cairo::SolidPattern> gray_color = Cairo::SolidPattern::create_rgb(0.6, 0.6, 0.6);

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
  cr->paint(); // fill image with the color

  // Draw labels
  cr->set_source(gray_color);
  cr->set_font_face(label_font);
  cr->set_font_size(12.0);

  // BOOST label
  cr->get_text_extents(BOOST_LABEL, extents);
  cr->move_to(64-(extents.width/2 + extents.x_bearing), 12);
  cr->show_text(BOOST_LABEL);

  // IAT label
  cr->get_text_extents(IAT_LABEL, extents);
  cr->move_to(20-(extents.width/2 + extents.x_bearing), 110);
  cr->show_text(IAT_LABEL);

  // P.MAX label
  cr->get_text_extents(PMAX_LABEL, extents);
  cr->move_to(70-(extents.width/2 + extents.x_bearing), 110);
  cr->show_text(PMAX_LABEL);

  // Knock label
  cr->get_text_extents(KNOCK_LABEL, extents);
  cr->move_to(118-(extents.width/2 + extents.x_bearing), 110);
  cr->show_text(KNOCK_LABEL);

  cr->restore();
}

void draw_numbers(Cairo::RefPtr<Cairo::Context> cr, float boost_psi_current, float boost_psi_max, int iat, int knock) {
  cr->save();

  cr->set_source(white_color);
  cr->set_font_face(number_font);

  // Boost
  snprintf(boost_psi_current_formatted, 7, "% 3.1f", boost_psi_current);
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

  // IAT
  snprintf(iat_formatted, 5, "% 3d", iat);
  if (iat > IAT_HOT_THRESHOLD)
    cr->set_source(red_color);
  else if (iat < IAT_COLD_THRESHOLD)
    cr->set_source(blue_color);
  else
    cr->set_source(green_color);
  cr->set_font_size(16.0);
  cr->get_text_extents(iat_formatted, extents);
  cr->move_to(IAT_X_CENTER-(extents.width/2 + extents.x_bearing), 126);
  cr->show_text(iat_formatted);

  // P. MAX
  snprintf(boost_psi_max_formatted, 6, "% 2.1f", boost_psi_max);
  if (boost_psi_max < 0)
    cr->set_source(blue_color);
  else if (boost_psi_max < BOOST_PSI_MAX)
    cr->set_source(green_color);
  else
    cr->set_source(red_color);
  cr->get_text_extents(boost_psi_max_formatted, extents);
  cr->move_to(PMAX_X_CENTER-(extents.width/2 + extents.x_bearing), 126);
  cr->show_text(boost_psi_max_formatted);

  // Knock
  snprintf(knock_formatted, 4, "%2d", knock);
  if (knock > KNOCK_PROBLEM_THRESHOLD)
    cr->set_source(red_color);
  else
    cr->set_source(green_color);
  cr->get_text_extents(knock_formatted, extents);
  cr->move_to(KNOCK_X_CENTER-(extents.width/2 + extents.x_bearing), 126);
  cr->show_text(knock_formatted);

  cr->restore();
}

void draw_graph(Cairo::RefPtr<Cairo::Context> cr, float boost_psi_current) {
  cr->save();

  for (int i = 0; i < 128; i = i+1) {
    if (i % 51 <= 25) {
      cr->set_source(blue_color);
    } else {
      cr->set_source(green_color);
    }
    cr->move_to(i, 72);
    cr->line_to(i, 72 + 25 - (i % 51));
    cr->stroke();
  }
  cr->set_source(white_color);
  cr->move_to(0, 73);
  cr->line_to(128, 73);
  cr->set_line_width(1.0);
  cr->set_antialias(Cairo::Antialias::ANTIALIAS_NONE);
  cr->stroke();
  cr->restore();
}

void render(Cairo::RefPtr<Cairo::ImageSurface> surface, float boost_psi_current, float boost_psi_max, int iat, int knock) {
  Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);

  cr->save(); // save the state of the context

  draw_gauge_background(cr);
  draw_numbers(cr, boost_psi_current, boost_psi_max, iat, knock);
  draw_graph(cr, boost_psi_current);

}

int main()
{
  #ifdef __linux__
  struct fb_var_screeninfo screen_info;
  struct fb_fix_screeninfo fixed_info;
  unsigned char *buffer = NULL;
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
        buffer = (unsigned char *) mmap(NULL,
                      buflen,
                      PROT_READ|PROT_WRITE,
                      MAP_SHARED,
                      fd,
                      0);
        if (buffer != MAP_FAILED)
        {
          /*
            * TODO: something interesting here.
            * "buffer" now points to screen pixels.
            * Each individual pixel might be at:
            *    buffer + x * screen_info.bits_per_pixel/8
            *           + y * fixed_info.line_length
            * Then you can write pixels at locations such as that.
            */

            Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(buffer, Cairo::FORMAT_RGB16_565, 128, 128, Cairo::ImageSurface::format_stride_for_width(Cairo::FORMAT_RGB16_565, screen_info.xres_virtual));

            render(surface, 19.7, 20.1, 135, 0);

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
  if (buffer && buffer != MAP_FAILED)
    munmap(buffer, buflen);
  if (fd >= 0)
    close(fd);

  return r;
#else
    Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 128, 128);
    render(surface, -18.2, 9.1, 81, 3);

#ifdef CAIRO_HAS_PNG_FUNCTIONS
      std::string filename = "image.png";
      surface->write_to_png(filename);

      std::cout << "Wrote png file \"" << filename << "\"" << std::endl;
#endif
#endif
}
