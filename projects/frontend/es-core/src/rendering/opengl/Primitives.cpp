//
// Created by bkg2k on 01/10/2020.
//

#ifdef USE_LEGACY_GL1_PRIMITIVES

#include "utils/math/Misc.h"
#include "Primitives.h"

// DEAD CODE — nothing calls Primitives:: anymore, and it could not work if it
// did: the fixed-pipeline calls below (glEnableClientState / glVertexPointer /
// glColorPointer) only compile because Primitives.h pulls in the GLES 1.x
// header, and do not exist on the GLES2 / GLES3 / GL Core contexts the
// renderer actually creates.
//
// If you ever revive this, route the blending through
// RendererGLCommon::DoEnableBlend/DoDisableBlend instead of calling glEnable
// directly: the renderer caches blend state, and a raw glDisable(GL_BLEND)
// here would desynchronise that cache and silently kill transparency in every
// draw that follows. See VULKAN.md.


void Primitives::DrawRectangle(int x, int y, int w, int h, Colors::ColorARGB color)
{
  GLNative points[12];
  points[0] = points[2] = points[8] = (int)x;
  points[1] = points[5] = points[7] = (int)y;
  points[3] = points[9] = points[11] = (int)y + h;
  points[4] = points[6] = points[10] = (int)x + w;

  DrawNonTexturedTriangles(points, color, 6);
}

void Primitives::DrawRectangle(const Rectangle& area, Colors::ColorARGB color)
{
  GLNative points[12];
  points[0] = points[2] = points[8] = (int)area.Left();
  points[1] = points[5] = points[7] = (int)area.Top();
  points[3] = points[9] = points[11] = (int)area.Bottom();
  points[4] = points[6] = points[10] = (int)area.Right();

  DrawNonTexturedTriangles(points, color, 6);
}

void Primitives::DrawRectangle(float x, float y, float w, float h, Colors::ColorARGB color)
{
  GLNative fl = (GLNative)Math::roundi(x);
  GLNative ft = (GLNative)Math::roundi(y);
  GLNative fr = (GLNative)(fl + Math::roundi(w));
  GLNative fb = (GLNative)(ft + Math::roundi(h));

  GLNative points[12];
  points[0] = points[2] = points[8] = fl;
  points[1] = points[5] = points[7] = ft;
  points[3] = points[9] = points[11] = fb;
  points[4] = points[6] = points[10] = fr;

  DrawNonTexturedTriangles(points, color, 6);
}

void Primitives::DrawRectangle(int x, int y, int w, int h, Colors::ColorARGB color, GLenum blendSourceFactor, GLenum blendDestinationFactor)
{
  GLNative points[12];
  points[0] = points[2] = points[8] = (int)x;
  points[1] = points[5] = points[7] = (int)y;
  points[3] = points[9] = points[11] = (int)y + h;
  points[4] = points[6] = points[10] = (int)x + w;

  DrawNonTexturedTriangles(points, color, 6, blendSourceFactor, blendDestinationFactor);
}

void Primitives::DrawRectangle(const Rectangle& area, Colors::ColorARGB color, GLenum blendSourceFactor, GLenum blendDestinationFactor)
{
  GLNative points[12];
  points[0] = points[2] = points[8] = (int)area.Left();
  points[1] = points[5] = points[7] = (int)area.Top();
  points[3] = points[9] = points[11] = (int)area.Bottom();
  points[4] = points[6] = points[10] = (int)area.Right();

  DrawNonTexturedTriangles(points, color, 6, blendSourceFactor, blendDestinationFactor);
}

void Primitives::DrawRectangle(float x, float y, float w, float h, Colors::ColorARGB color, GLenum blendSourceFactor, GLenum blendDestinationFactor)
{
  GLNative fl = (GLNative)Math::roundi(x);
  GLNative ft = (GLNative)Math::roundi(y);
  GLNative fr = (GLNative)(fl + Math::roundi(w));
  GLNative fb = (GLNative)(ft + Math::roundi(h));

  GLNative points[12];
  points[0] = points[2] = points[8] = fl;
  points[1] = points[5] = points[7] = ft;
  points[3] = points[9] = points[11] = fb;
  points[4] = points[6] = points[10] = fr;

  DrawNonTexturedTriangles(points, color, 6, blendSourceFactor, blendDestinationFactor);
}

void Primitives::DrawNonTexturedTriangles(Primitives::GLNative* vertex, Colors::ColorARGB color, int vertexCount)
{
  GLubyte colors[6 * 4];
  Colors::FillGPUColorArray(colors, color, 6);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);

  glVertexPointer(2, GLNativeType, 0, vertex);
  glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);

  glDrawArrays(GL_TRIANGLES, 0, vertexCount);

  glDisable(GL_BLEND);
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
}

void Primitives::DrawNonTexturedTriangles(Primitives::GLNative* vertex, Colors::ColorARGB color, int vertexCount,
                                          GLenum blendSourceFactor, GLenum blendDestinationFactor)
{
  GLubyte colors[6 * 4];
  Colors::FillGPUColorArray(colors, color, 6);

  glEnable(GL_BLEND);
  glBlendFunc(blendSourceFactor, blendDestinationFactor);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);

  glVertexPointer(2, GLNativeType, 0, vertex);
  glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);

  glDrawArrays(GL_TRIANGLES, 0, vertexCount);

  glDisable(GL_BLEND);
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
}



#endif // USE_LEGACY_GL1_PRIMITIVES
