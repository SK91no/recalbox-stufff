//
// Created by bkg2k on 27/08/24.
//

#include "NinePatchImage.h"
#include "Renderer.h"

void NinePatchImage::PrepareDrawingTo(const Rectangle& targetArea)
{
  if (!mImage.MakeReady()) return;

  const Vector2f ts = mImage.Size().toFloat();
  float chunkWidth = Math::round(ts.x() / 3.f);
  float chunkHeight = Math::round(ts.y() / 3.f);

  // coordinates on the image in pixels, top left origin
  const Vector2f pieceCoords[9] = {
    Vector2f(0, 0),
    Vector2f(chunkWidth * 1.f, 0),
    Vector2f(chunkWidth * 2.f, 0),
    Vector2f(0, chunkHeight * 1.f),
    Vector2f(chunkWidth * 1.f, chunkHeight * 1.f),
    Vector2f(chunkWidth * 2.f, chunkHeight * 1.f),
    Vector2f(0, chunkHeight * 2.f),
    Vector2f(chunkWidth * 1.f, chunkHeight * 2.f),
    Vector2f(chunkWidth * 2.f, chunkHeight * 2.f),
  };

  // corners never stretch, so we calculate a width and height for slices 1, 3, 5, and 7
  float borderWidth = targetArea.Width() - (chunkWidth * 2);   //should be pieceSizes[0] and pieceSizes[2]
  float borderHeight = targetArea.Height() - (chunkHeight * 2); //should be pieceSizes[0] and pieceSizes[6]

  mVertices[0 * Vertex::sVertexPerRectangle].Target = pieceCoords[0]; //top left
  mVertices[1 * Vertex::sVertexPerRectangle].Target = pieceCoords[1]; //top middle
  mVertices[2 * Vertex::sVertexPerRectangle].Target = pieceCoords[1] + Vector2f(borderWidth, 0); //top right

  mVertices[3 * Vertex::sVertexPerRectangle].Target = mVertices[0 * Vertex::sVertexPerRectangle].Target + Vector2f(0, chunkHeight); //mid left
  mVertices[4 * Vertex::sVertexPerRectangle].Target = mVertices[3 * Vertex::sVertexPerRectangle].Target + Vector2f(chunkWidth, 0); //mid middle
  mVertices[5 * Vertex::sVertexPerRectangle].Target = mVertices[4 * Vertex::sVertexPerRectangle].Target + Vector2f(borderWidth, 0); //mid right

  mVertices[6 * Vertex::sVertexPerRectangle].Target = mVertices[3 * Vertex::sVertexPerRectangle].Target + Vector2f(0, borderHeight); //bot left
  mVertices[7 * Vertex::sVertexPerRectangle].Target = mVertices[6 * Vertex::sVertexPerRectangle].Target + Vector2f(chunkWidth, 0); //bot middle
  mVertices[8 * Vertex::sVertexPerRectangle].Target = mVertices[7 * Vertex::sVertexPerRectangle].Target + Vector2f(borderWidth, 0); //bot right

  int v = 0;
  for (int slice = 0; slice < 9; slice++)
  {
    Vector2f size(0, 0);

    // corners
    if (slice == 0 || slice == 2 || slice == 6 || slice == 8) size.Set(chunkWidth, chunkHeight);
    // vertical borders
    if (slice == 1 || slice == 7) size.Set(borderWidth, chunkHeight);
    // horizontal borders
    if (slice == 3 || slice == 5) size.Set(chunkWidth, borderHeight);
    // center
    if (slice == 4) size.Set(borderWidth, borderHeight);

    // no resizing will be necessary
    // mVertices[v + 0] is already correct
    mVertices[v + 1].Target = mVertices[v + 0].Target + size;
    mVertices[v + 2].Target.Set(mVertices[v + 0].Target.X, mVertices[v + 1].Target.Y);

    mVertices[v + 3].Target.Set(mVertices[v + 1].Target.X, mVertices[v + 0].Target.Y);
    mVertices[v + 4].Target = mVertices[v + 1].Target;
    mVertices[v + 5].Target = mVertices[v + 0].Target;

    // texture coordinates
    // the y = (1 - y) is to deal with texture coordinates having a bottom left corner origin vs. verticies having a top left origin
    mVertices[v + 0].Source.Set(pieceCoords[slice].x() / ts.x(), 1 - (pieceCoords[slice].y() / ts.y()));
    mVertices[v + 1].Source.Set((pieceCoords[slice].x() + chunkWidth) / ts.x(),
                                1 - ((pieceCoords[slice].y() + chunkHeight) / ts.y()));
    mVertices[v + 2].Source.Set(mVertices[v + 0].Source.X, mVertices[v + 1].Source.Y);

    mVertices[v + 3].Source.Set(mVertices[v + 1].Source.X, mVertices[v + 0].Source.Y);
    mVertices[v + 4].Source = mVertices[v + 1].Source;
    mVertices[v + 5].Source = mVertices[v + 0].Source;

    v += Vertex::sVertexPerRectangle;
  }

  // round vertices
  for (int i = Vertex::sVertexPerRectangle * 9; --i >= 0;)
    mVertices[i].Move(targetArea.Left(), targetArea.Top()).Round();
}

void NinePatchImage::Render(const Rectangle& targetArea, Colors::ColorRGBA color)
{
  if (targetArea != mLastTarget)
    PrepareDrawingTo(targetArea);
  if (mImage.MakeReady())
    Renderer::DrawTexturedTriangles(mImage.Identifier(), mVertices, color, Vertex::sVertexPerRectangle * 9, false);
}
