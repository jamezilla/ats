/*
my_curve.h

The Gtk_My_Curve Widget is only an "alias" of the original Gtk_Curve one by the GIMP Team.
I am just trying to fix some bugs of it...
*/

#ifndef __GTK_MY_CURVE_H__
#define __GTK_MY_CURVE_H__

#include <gdk/gdk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_MY_CURVE                  (gtk_my_curve_get_type ())
#define GTK_MY_CURVE(obj)                  (GTK_CHECK_CAST ((obj), GTK_TYPE_MY_CURVE, GtkMyCurve))
#define GTK_MY_CURVE_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_MY_CURVE, GtkMyCurveClass))
#define GTK_IS_MY_CURVE(obj)               (GTK_CHECK_TYPE ((obj), GTK_TYPE_MY_CURVE))
#define GTK_IS_MY_CURVE_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_MY_CURVE))


typedef struct _GtkCurve	GtkMyCurve;
typedef struct _GtkCurveClass	GtkMyCurveClass;


struct _GtkMyCurve
{
  GtkDrawingArea graph;

  gint cursor_type;
  gfloat min_x;
  gfloat max_x;
  gfloat min_y;
  gfloat max_y;
  GdkPixmap *pixmap;
  GtkCurveType curve_type;
  gint height;                  /* (cached) graph height in pixels */
  gint grab_point;              /* point currently grabbed */
  gint last;

  /* (cached) curve points: */
  gint num_points;
  GdkPoint *point;

  /* control points: */
  gint num_ctlpoints;           /* number of control points */
  gfloat (*ctlpoint)[2];        /* array of control points */
};

struct _GtkMyCurveClass
{
  GtkDrawingAreaClass parent_class;

  void (* curve_type_changed) (GtkMyCurve *my_curve);
};


GtkType	gtk_my_curve_get_type	(void);
GtkWidget*	gtk_my_curve_new		(void);
void		gtk_my_curve_reset	(GtkMyCurve *my_curve);
void		gtk_my_curve_set_gamma	(GtkMyCurve *my_curve, gfloat gamma);
void		gtk_my_curve_set_range	(GtkMyCurve *my_curve,
					 gfloat min_x, gfloat max_x,
					 gfloat min_y, gfloat max_y);
void		gtk_my_curve_get_vector	(GtkMyCurve *my_curve,
					 int veclen, gfloat vector[]);
void		gtk_my_curve_set_vector	(GtkMyCurve *my_curve,
					 int veclen, gfloat vector[]);

void		gtk_my_curve_set_curve_type (GtkMyCurve *my_curve, GtkCurveType type);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_MY_CURVE_H__ */
