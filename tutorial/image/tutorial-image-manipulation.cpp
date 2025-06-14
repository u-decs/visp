/*! \example tutorial-image-manipulation.cpp */
#include <visp3/core/vpConfig.h>
#include <visp3/core/vpImage.h>

int main()
{
#ifdef ENABLE_VISP_NAMESPACE
  using namespace VISP_NAMESPACE_NAME;
#endif

  try {
    vpImage<unsigned char> gray_image(240, 320);
    vpImage<vpRGBa> color_image(240, 320);

    gray_image = 128;
    vpRGBa color(255, 0, 0);
    color_image = color;

    unsigned int igray_max = gray_image.getHeight() - 1;
    unsigned int jgray_max = gray_image.getWidth() - 1;
    std::cout << "Gray  image, last pixel intensity: " << static_cast<int>(gray_image[igray_max][jgray_max]) << std::endl;

    unsigned int icolor_max = color_image.getHeight() - 1;
    unsigned int jcolor_max = color_image.getWidth() - 1;
    std::cout << "Color image, last pixel RGB components: "
      << static_cast<int>(color_image[icolor_max][jcolor_max].R) << " "
      << static_cast<int>(color_image[icolor_max][jcolor_max].G) << " "
      << static_cast<int>(color_image[icolor_max][jcolor_max].B) << std::endl;
  }
  catch (const vpException &e) {
    std::cout << "Catch an exception: " << e << std::endl;
  }
}
