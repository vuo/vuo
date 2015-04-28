/**
 * @file
 * VuoInputEditorCurveRenderer interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORCURVERENDERER_HH
#define VUOINPUTEDITORCURVERENDERER_HH

extern "C"
{
#include "VuoCurve.h"
#include "VuoWave.h"
#include "VuoWrapMode.h"
#include "VuoLoopType.h"
#include "VuoSizingMode.h"
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused"

/**
 * Renders the specified @c render block onto a 15x16 point icon, respecting the device pixel ratio.
 */
static QIcon *renderMenuIcon(void (^render)(QPainter &p))
{
	qreal devicePixelRatio = qApp->devicePixelRatio();
	QPixmap px(15*devicePixelRatio,16*devicePixelRatio);
	px.setDevicePixelRatio(devicePixelRatio);
	px.fill(Qt::transparent);

	QPainter p(&px);
	p.setRenderHint(QPainter::Antialiasing);
	render(p);

	return new QIcon(px);
}

/**
 * Renders an icon representing the specified curve.
 */
static QIcon *renderMenuIconWithCurve(VuoCurve curve, VuoCurveEasing easing=VuoCurveEasing_In)
{
	return renderMenuIcon(^(QPainter &p){
							  QPainterPath path;
							  path.moveTo(0,15.5);
							  for (VuoReal x = 0; x <= 14; ++x)
								  path.lineTo(0.5+x, 15.5-VuoReal_curve(x, 0, 14, 14, curve, easing, VuoLoopType_None));
							  p.strokePath(path, QPen(Qt::black, 1.));
						  });
}

/**
 * Renders an icon representing the specified wave.
 */
static QIcon *renderMenuIconWithWave(VuoWave wave)
{
	return renderMenuIcon(^(QPainter &p){
							  QPainterPath path;
							  path.moveTo(0,15.5);

							  if (wave == VuoWave_Sine)
								  for (VuoReal x = 0; x <= 14; ++x)
									  path.lineTo(0.5+x, 7.5+7.*cos(2.*M_PI*x/14.));
							  else if (wave == VuoWave_Triangle)
								  path.lineTo(7.5, 1.0);
							  else if (wave == VuoWave_Sawtooth)
								  path.lineTo(14.5, 1.0);

							  path.lineTo(14.5, 15.0);
							  p.strokePath(path, QPen(Qt::black, 1.));
						  });
}

/**
 * Renders an icon representing the specified wrap mode.
 */
static QIcon *renderMenuIconWithWrapMode(VuoWrapMode wrapMode)
{
	return renderMenuIcon(^(QPainter &p){
							  QPainterPath path;
							  path.moveTo(0.0, 10.5);
							  if (wrapMode == VuoWrapMode_Wrap)
							  {
								  path.lineTo(4.0, 5.5);
								  path.moveTo(5.0, 10.5);
								  path.lineTo(9.0, 5.5);
								  path.moveTo(10.0, 10.5);
								  path.lineTo(14.0, 5.5);
							  }
							  else if (wrapMode == VuoWrapMode_Saturate)
							  {
								  path.lineTo(5, 10.5);
								  path.lineTo(10, 5.5);
								  path.lineTo(14.5, 5.5);
							  }
							  p.strokePath(path, QPen(Qt::black, 1.));
						  });
}

/**
 * Renders an icon representing the specified loop type.
 */
static QIcon *renderMenuIconWithLoopType(VuoLoopType loopType)
{
	return renderMenuIcon(^(QPainter &p){
							  QPainterPath path;
							  path.moveTo(0.0, 10.5);
							  if (loopType == VuoLoopType_Loop)
							  {
								  path.lineTo(4.0, 5.5);
								  path.moveTo(5.0, 10.5);
								  path.lineTo(9.0, 5.5);
								  path.moveTo(10.0, 10.5);
								  path.lineTo(14.0, 5.5);
							  }
							  else if (loopType == VuoLoopType_Mirror)
							  {
								  path.lineTo(4.0, 5.5);
								  path.lineTo(5.0, 5.5);
								  path.lineTo(9.0, 10.5);
								  path.lineTo(10.0, 10.5);
								  path.lineTo(14.0, 5.5);
							  }
							  else if (loopType == VuoLoopType_None)
								  path.lineTo(4.0, 5.5);
							  p.strokePath(path, QPen(Qt::black, 1.));
						  });
}

/**
 * Renders an icon representing the specified sizing mode.
 */
static QIcon *renderMenuIconWithSizingMode(VuoSizingMode sizingMode)
{
	return renderMenuIcon(^(QPainter &p){
							  QPainterPath grayPath, blackPath;
							  if (sizingMode == VuoSizingMode_Stretch)
								  blackPath.addRect(QRectF(1.0, 5.0, 13.0, 5.0));
							  else if (sizingMode == VuoSizingMode_Fit)
							  {
								  grayPath.addRect(QRectF(0.5, 4.5, 14.0, 6.0));
								  blackPath.addRect(QRectF(5.0, 5.0, 5.0, 5.0));
							  }
							  else if (sizingMode == VuoSizingMode_Fill)
							  {
								  grayPath.addRect(QRectF(0.5, 4.5, 14.0, 6.0));
								  blackPath.addRect(QRectF(1.0, 1.0, 13.0, 13.0));
							  }
							  p.strokePath(grayPath, QPen(Qt::gray, 1.));
							  p.strokePath(blackPath, QPen(Qt::black, 2.));
						  });
}

#pragma clang diagnostic pop

#endif // VUOINPUTEDITORCURVERENDERER_HH
