#include "components/NinePatchComponent.h"
#include "WindowManager.h"
#include "utils/Log.h"
#include "Renderer.h"
#include "themes/ThemeData.h"
#include <rendering/textures/TextureManager.h>

NinePatchComponent::NinePatchComponent(WindowManager& window)
  : ThemableComponent(window)
  , mVertices()
  , mColors()
  , mW(0)
  , mH(0)
  , mEdgeColor(0xFFFFFFFF)
  , mCenterColor(0xFFFFFFFF)
{
}

NinePatchComponent::NinePatchComponent(WindowManager& window, const Path& path)
	: NinePatchComponent(window)
{
  mPath = path;
	if (!mPath.IsEmpty())
		buildVertices();
}

void NinePatchComponent::updateColors()
{
	Renderer::BuildGLColorArray(mColors, mEdgeColor, 6 * 9);
	Renderer::BuildGLColorArray(&mColors[4 * 6 * 4], mCenterColor, 6);
}

void NinePatchComponent::buildVertices()
{
	mTexture = TextureManager::Instance().Create(mPath, TextureHolder::Properties::ImmediateLoad);
  TextureManager::Instance().RequestTextureLoading(mTexture, true);

	if (mTexture.Size() == Vector2i::Zero())
	{
    { LOG(LogWarning) << "[NinePatchComponent] missing texture!"; }
		return;
	}

	updateColors();

	const Vector2f ts = mTexture.Size().toFloat();
	mW = ts.x() / 3.f;
  mH = ts.y() / 3.f;

	//coordinates on the image in pixels, top left origin
	const Vector2f pieceCoords[9] = {
		Vector2f(       0,        0),
		Vector2f(mW * 1.f,        0),
		Vector2f(mW * 2.f,        0),
		Vector2f(       0, mH * 1.f),
		Vector2f(mW * 1.f, mH * 1.f),
		Vector2f(mW * 2.f, mH * 1.f),
		Vector2f(       0, mH * 2.f),
		Vector2f(mW * 1.f, mH * 2.f),
		Vector2f(mW * 2.f, mH * 2.f),
	};

	//corners never stretch, so we calculate a width and height for slices 1, 3, 5, and 7
	float borderWidth = mSize.x() - (mW * 2); //should be pieceSizes[0] and pieceSizes[2]
	//if(borderWidth < pieceSizes.x())
	//	borderWidth = pieceSizes.x();

	float borderHeight = mSize.y() - (mH * 2); //should be pieceSizes[0] and pieceSizes[6]
	//if(borderHeight < pieceSizes.y())
	//	borderHeight = pieceSizes.y();

	mVertices[0 * 6].Target = pieceCoords[0]; //top left
	mVertices[1 * 6].Target = pieceCoords[1]; //top middle
	mVertices[2 * 6].Target = pieceCoords[1] + Vector2f(borderWidth, 0); //top right

	mVertices[3 * 6].Target = mVertices[0 * 6].Target + Vector2f(0, mH); //mid left
	mVertices[4 * 6].Target = mVertices[3 * 6].Target + Vector2f(mW, 0); //mid middle
	mVertices[5 * 6].Target = mVertices[4 * 6].Target + Vector2f(borderWidth, 0); //mid right

	mVertices[6 * 6].Target = mVertices[3 * 6].Target + Vector2f(0, borderHeight); //bot left
	mVertices[7 * 6].Target = mVertices[6 * 6].Target + Vector2f(mW, 0); //bot middle
	mVertices[8 * 6].Target = mVertices[7 * 6].Target + Vector2f(borderWidth, 0); //bot right

	int v = 0;
	for (int slice = 0; slice < 9; slice++)
	{
		Vector2f size(0, 0);

		//corners
		if(slice == 0 || slice == 2 || slice == 6 || slice == 8) size.Set(mW, mH);
		//vertical borders
		if(slice == 1 || slice == 7) size.Set(borderWidth, mH);
		//horizontal borders
		if(slice == 3 || slice == 5) size.Set(mW, borderHeight);

		//center
		if(slice == 4) size.Set(borderWidth, borderHeight);

		//no resizing will be necessary
		//mVertices[v + 0] is already correct
		mVertices[v + 1].Target = mVertices[v + 0].Target + size;
		mVertices[v + 2].Target.Set(mVertices[v + 0].Target.X, mVertices[v + 1].Target.Y);

		mVertices[v + 3].Target.Set(mVertices[v + 1].Target.X, mVertices[v + 0].Target.Y);
		mVertices[v + 4].Target = mVertices[v + 1].Target;
		mVertices[v + 5].Target = mVertices[v + 0].Target;

		//texture coordinates
		//the y = (1 - y) is to deal with texture coordinates having a bottom left corner origin vs. verticies having a top left origin
		mVertices[v + 0].Source.Set(pieceCoords[slice].x() / ts.x(), 1 - (pieceCoords[slice].y() / ts.y()));
		mVertices[v + 1].Source.Set((pieceCoords[slice].x() + mW) / ts.x(), 1 - ((pieceCoords[slice].y() + mH) / ts.y()));
		mVertices[v + 2].Source.Set(mVertices[v + 0].Source.X, mVertices[v + 1].Source.Y);

		mVertices[v + 3].Source.Set(mVertices[v + 1].Source.X, mVertices[v + 0].Source.Y);
		mVertices[v + 4].Source = mVertices[v + 1].Source;
		mVertices[v + 5].Source = mVertices[v + 0].Source;

		v += 6;
	}

	// round vertices
	for(int i = 6*9; --i >= 0; )
	{
		mVertices[i].Target.Round();
	}
}

void NinePatchComponent::Render(const Transform4x4f& parentTrans)
{
  if (mThemeDisabled) return;

  Transform4x4f trans = (parentTrans * getTransform()).round();
	
	if (mTexture.Valid())
	{
		Renderer::SetMatrix(trans);

    if (mTexture.MakeReady())
    	mTexture.Render(mVertices, mColors, 6 * 9, false);
	}

	renderChildren(trans);
}

void NinePatchComponent::onSizeChanged()
{
	buildVertices();
}

void NinePatchComponent::fitTo(Vector2f size, Vector3f position, Vector2f padding)
{
	size += padding;
	position[0] -= padding.x() / 2;
	position[1] -= padding.y() / 2;

	setSize(size + Vector2f(mW * 2, mH * 2));
	setPosition(-mW + position.x(), -mH + position.y());
}

void NinePatchComponent::setImagePath(const Path& path)
{
  if (mPath != path)
  {
    mPath = path;
    buildVertices();
  }
}

void NinePatchComponent::setEdgeColor(unsigned int edgeColor)
{
  if (edgeColor != mEdgeColor)
  {
    mEdgeColor = edgeColor;
    updateColors();
  }
}

void NinePatchComponent::setCenterColor(unsigned int centerColor)
{
  if (centerColor != mCenterColor)
  {
    mCenterColor = centerColor;
    updateColors();
  }
}

void NinePatchComponent::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
	if(hasFlag(properties, ThemePropertyCategory::Path))
		setImagePath(element.HasProperty(ThemePropertyName::Path) ? element.AsPath(ThemePropertyName::Path) : Path::Empty);
}
