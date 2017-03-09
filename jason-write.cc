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

void draw_gauge_background(Cairo::RefPtr<Cairo::Context> cr) {
  cr->save();
  cr->paint(); // fill image with the color

  // Draw labels
  cr->set_source_rgb(0.6, 0.6, 0.6);
  Cairo::RefPtr<Cairo::ToyFontFace> label_font =
    Cairo::ToyFontFace::create("Avenir Next",
                               Cairo::FONT_SLANT_NORMAL,
                               Cairo::FONT_WEIGHT_BOLD);
  cr->set_font_face(label_font);
  cr->set_font_size(12.0);

  // BOOST label
  cr->move_to(28, 12);
  cr->show_text("BOOST (PSI)");

  // IAT label
  cr->move_to(10, 110);
  cr->show_text("IAT");

  // P.MAX label
  cr->move_to(53, 110);
  cr->show_text("P.MAX");

  // Knock label
  cr->move_to(114, 110);
  cr->show_text("K");
  cr->restore();
}

void draw_numbers(Cairo::RefPtr<Cairo::Context> cr) {
  cr->save();

  cr->set_source_rgb(1.0, 1.0, 1.0);
  Cairo::RefPtr<Cairo::ToyFontFace> number_font =
    Cairo::ToyFontFace::create("Eurostile",
                               Cairo::FONT_SLANT_NORMAL,
                               Cairo::FONT_WEIGHT_BOLD);
  cr->set_font_face(number_font);

  // Boost
  cr->set_source_rgb(0.14, 0.9, 0.5);
  cr->set_font_size(45.0);
  cr->move_to(16, 45);
  cr->show_text("20.2");

  // IAT
  cr->set_source_rgb(0.83, 0.0, 0.6);
  cr->set_font_size(16.0);
  cr->move_to(4, 126);
  cr->show_text("189");

  // P. MAX
  cr->set_source_rgb(0.14, 0.9, 0.5);
  cr->move_to(57, 126);
  cr->show_text("20.9");

  // Knock
  cr->set_source_rgb(0.14, 0.9, 0.5);
  cr->move_to(114, 126);
  cr->show_text("0");

  cr->restore();
}

void draw_graph(Cairo::RefPtr<Cairo::Context> cr) {
  cr->save();

  for (int i = 0; i < 128; i = i+1) {
    if (i % 51 <= 25) {
      cr->set_source_rgb(0.38, 0.66, 1.0);
    } else {
      cr->set_source_rgb(0.14, 0.9, 0.5);
    }
    cr->move_to(i, 72);
    cr->line_to(i, 72 + 25 - (i % 51));
    cr->stroke();
  }
  cr->set_source_rgb(1.0, 1.0, 1.0);
  cr->move_to(0, 73);
  cr->line_to(128, 73);
  cr->set_line_width(1.0);
  cr->set_antialias(Cairo::Antialias::ANTIALIAS_NONE);
  cr->stroke();
  cr->restore();
}

void render(Cairo::RefPtr<Cairo::ImageSurface> surface) {
  Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surface);

  cr->save(); // save the state of the context

  draw_gauge_background(cr);
  draw_numbers(cr);
  draw_graph(cr);

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

            render(surface);

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
    Cairo::RefPtr<Cairo::ImageSurface> surface =
        Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 128, 128);
    render(surface);

#ifdef CAIRO_HAS_PNG_FUNCTIONS
      std::string filename = "image.png";
      surface->write_to_png(filename);

      std::cout << "Wrote png file \"" << filename << "\"" << std::endl;
#endif
#endif
}
