#ifndef BLOCKBUSTER_GTK_H
#define BLOCKBUSTER_GTK_H

#include "ui.h"

extern UserInterface gtkUserInterface;
void gtk_Resize(Canvas *canvas, int newWidth, int newHeight, int cameFromX);
void gtk_Move(Canvas *canvas, int newX, int newY, int cameFromX);
#endif
