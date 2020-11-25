#include "mainwindow.h"
#include <QApplication>
#include <librteye2_includes.hpp>
#include <QDesktopWidget>



int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  
  QSize availableSize = a.desktop()->availableGeometry().size();
  int width = availableSize.width();
  int height = availableSize.height();
  std::cout << width << " " << height << std::endl;
  

    
  MainWindow w;
  w.show();
  
  
  
  auto blah = a.exec();


  
  
  return blah;
}
