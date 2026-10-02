//
// Created by bkg2k on 15/09/2020.
//

#include "Mapper.h"
#include "Colors.h"

void Mapper::MapTexture(const Rectangle& target, const Rectangle& texture, float textureWidth, float textureHeight, Vertex* vertexes)
{
  // First target triangle and opposite corner
  vertexes[0].Target.X =
  vertexes[1].Target.X = target.Left();
  vertexes[2].Target.X =
  vertexes[5].Target.X = target.Left() + target.Width();
  vertexes[0].Target.Y =
  vertexes[2].Target.Y = target.Top();
  vertexes[1].Target.Y =
  vertexes[5].Target.Y = target.Top() + target.Height();

  // First texture triangle and opposite corner
  vertexes[0].Source.X =
  vertexes[1].Source.X = texture.Left() / textureWidth;
  vertexes[2].Source.X =
  vertexes[5].Source.X = (texture.Left() + texture.Width()) / textureWidth;
  vertexes[0].Source.Y =
  vertexes[2].Source.Y = (textureHeight - texture.Top()) / textureHeight;
  vertexes[1].Source.Y =
  vertexes[5].Source.Y = (textureHeight - (texture.Top() + texture.Height())) / textureHeight;

  // Copy identical corners
  vertexes[3] = vertexes[2];
  vertexes[4] = vertexes[1];
}

void Mapper::MapTexture(const Rectangle& target, Vertex* vertexes)
{
  // First target triangle and opposite corner
  vertexes[0].Target.X =
  vertexes[1].Target.X = target.Left();
  vertexes[2].Target.X =
  vertexes[5].Target.X = target.Left() + target.Width();
  vertexes[0].Target.Y =
  vertexes[2].Target.Y = target.Top();
  vertexes[1].Target.Y =
  vertexes[5].Target.Y = target.Top() + target.Height();

  // First texture triangle and opposite corner
  vertexes[0].Source.X =
  vertexes[1].Source.X = 0.f;
  vertexes[2].Source.X =
  vertexes[5].Source.X = 1.0f;
  vertexes[0].Source.Y =
  vertexes[2].Source.Y = 1.0f;
  vertexes[1].Source.Y =
  vertexes[5].Source.Y = 0.f;

  // Copy identical corners
  vertexes[3] = vertexes[2];
  vertexes[4] = vertexes[1];
}

void Mapper::MapColors(unsigned int topleftColor, unsigned int toprightColor, unsigned int bottomrightColor,
                       unsigned int bottomleftColor, ColorByte* glColors)
{
  Colors::StoreGPUColor(&glColors[ 0], topleftColor);
  Colors::StoreGPUColor(&glColors[ 4], bottomleftColor);
  Colors::StoreGPUColor(&glColors[ 8], toprightColor);
  Colors::StoreGPUColor(&glColors[12], toprightColor);
  Colors::StoreGPUColor(&glColors[16], bottomleftColor);
  Colors::StoreGPUColor(&glColors[20], bottomrightColor);
}
