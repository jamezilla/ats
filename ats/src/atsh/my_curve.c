/*
my_curve.c
Oscar Pablo Di Liscia (odiliscia@unq.edu.ar)
N.B.: The Gtk_My_Curve Widget is only an "alias" of the original Gtk_Curve one by the GIMP Team.
I am just trying to fix some bugs of it...
See the gtk_my_curve_change_type function.
*/

/*UNCOMMENT THESE TO MAKE UNDER WINDOWS*/
//#include "stdlib.h"	
//#include "math.h"
/*COMMENT THESE TO MAKE UNDER WINDOWS*/
#include <string.h>	
#include <errno.h>
#include <stdlib.h>
#include <math.h>
//THESE FOUR ARE COMMON
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "my_curve.h"

#define RADIUS		3	/* radius of the control points */
#define MIN_DISTANCE	8	/* min distance between control points */

#define GRAPH_MASK	(GDK_EXPOSURE_MASK |		\
			 GDK_POINTER_MOTION_MASK |	\
			 GDK_POINTER_MOTION_HINT_MASK |	\
			 GDK_ENTER_NOTIFY_MASK |	\
			 GDK_BUTTON_PRESS_MASK |	\
			 GDK_BUTTON_RELEASE_MASK |	\
			 GDK_BUTTON1_MOTION_MASK)

enum {
  ARG_0,
  ARG_CURVE_TYPE,
  ARG_MIN_X,
  ARG_MAX_X,
  ARG_MIN_Y,
  ARG_MAX_Y
};

static GtkDrawingAreaClass *parent_class = NULL;
static guint curve_type_changed_signal = 0;


/* forward declarations: */
static void gtk_my_curve_class_init   (GtkMyCurveClass *class);
static void gtk_my_curve_init         (GtkMyCurve      *curve);
static void gtk_my_curve_set_arg     (GtkObject      *object,
				   GtkArg         *arg,
				   guint           arg_id);
static void gtk_my_curve_get_arg     (GtkObject      *object,
				   GtkArg         *arg,
				   guint           arg_id);
//static void gtk_my_curve_finalize     (GObject     *object);
static gint gtk_my_curve_graph_events (GtkWidget     *widget, 
				    GdkEvent      *event, 
				    GtkMyCurve      *c);
static void gtk_my_curve_size_graph   (GtkMyCurve      *curve);

GtkType
gtk_my_curve_get_type (void)
{
  static GtkType curve_type = 0;
  static const GtkTypeInfo my_curve_info = {
    "GtkMyCurve",
    sizeof (GtkMyCurve),
    sizeof (GtkMyCurveClass),
    (GtkClassInitFunc) gtk_my_curve_class_init,
    (GtkObjectInitFunc) gtk_my_curve_init,
    /* reserved_1 */ NULL,
    /* reserved_2 */ NULL,
    (GtkClassInitFunc) NULL,
  };

  if (!curve_type) curve_type = gtk_type_unique (GTK_TYPE_DRAWING_AREA, &my_curve_info);
  return curve_type;
}

static void
gtk_my_curve_class_init (GtkMyCurveClass *class)
{
  GtkObjectClass *object_class;
  
  parent_class = gtk_type_class (GTK_TYPE_DRAWING_AREA);
  
  object_class = (GtkObjectClass *) class;
  
  object_class->set_arg = gtk_my_curve_set_arg;
  object_class->get_arg = gtk_my_curve_get_arg;
  //  object_class->finalize = gtk_my_curve_finalize;
  
  curve_type_changed_signal =
    g_signal_new ("curve_type_changed", (GType)object_class, G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GtkMyCurveClass, curve_type_changed), NULL, NULL,
                  NULL, G_TYPE_NONE, 0);
//     gtk_signal_new ("curve_type_changed", GTK_RUN_FIRST, GTK_CLASS_TYPE(object_class),
// 		    GTK_SIGNAL_OFFSET (GtkMyCurveClass, curve_type_changed),
// 		    gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);
//   (const gchar *name,
//    GtkSignalRunType signal_flags,
//    GtkType object_type,
//    guint function_offset,
//    GtkSignalMarshaller marshaller,
//    GtkType return_val,
//    guint n_args,
//    ...);


  // gtk_object_class_add_signals (object_class, &curve_type_changed_signal, 1);
  
  gtk_object_add_arg_type ("GtkMyCurve::curve_type", GTK_TYPE_MY_CURVE,
			   GTK_ARG_READWRITE, ARG_CURVE_TYPE);
  gtk_object_add_arg_type ("GtkMyCurve::min_x", GTK_TYPE_FLOAT,
			   GTK_ARG_READWRITE, ARG_MIN_X);
  gtk_object_add_arg_type ("GtkMyCurve::max_x", GTK_TYPE_FLOAT,
			   GTK_ARG_READWRITE, ARG_MAX_X);
  gtk_object_add_arg_type ("GtkMyCurve::min_y", GTK_TYPE_FLOAT,
			   GTK_ARG_READWRITE, ARG_MIN_Y);
  gtk_object_add_arg_type ("GtkMyCurve::max_y", GTK_TYPE_FLOAT,
			   GTK_ARG_READWRITE, ARG_MAX_Y);
}

static void
gtk_my_curve_init (GtkCurve *my_curve)
{
  gint old_mask;

  my_curve->cursor_type = GDK_TOP_LEFT_ARROW;
  my_curve->pixmap = NULL;
  my_curve->curve_type = GTK_CURVE_TYPE_SPLINE;
  my_curve->height = 0;
  my_curve->grab_point = -1;

  my_curve->num_points = 0;
  my_curve->point = 0; 

  my_curve->num_ctlpoints = 0;
  my_curve->ctlpoint = NULL;

  my_curve->min_x = my_curve->min_y = 0.0;
  my_curve->max_x = my_curve->max_y = 1.0;

  old_mask = gtk_widget_get_events (GTK_WIDGET (my_curve));
  gtk_widget_set_events (GTK_WIDGET (my_curve), old_mask | GRAPH_MASK);
  g_signal_connect (G_OBJECT (my_curve), "event",
                    G_CALLBACK(gtk_my_curve_graph_events), my_curve);
  gtk_my_curve_size_graph (my_curve);
}

static void
gtk_my_curve_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
  GtkMyCurve *my_curve = GTK_MY_CURVE (object);
  
  switch (arg_id)
    {
    case ARG_CURVE_TYPE:
      gtk_my_curve_set_curve_type (my_curve, GTK_VALUE_ENUM (*arg));
      break;
    case ARG_MIN_X:
      gtk_my_curve_set_range (my_curve, GTK_VALUE_FLOAT (*arg), my_curve->max_x,
                              my_curve->min_y, my_curve->max_y);
      break;
    case ARG_MAX_X:
      gtk_my_curve_set_range (my_curve, my_curve->min_x, GTK_VALUE_FLOAT (*arg),
                              my_curve->min_y, my_curve->max_y);
      break;	
    case ARG_MIN_Y:
      gtk_my_curve_set_range (my_curve, my_curve->min_x, my_curve->max_x,
                              GTK_VALUE_FLOAT (*arg), my_curve->max_y);
      break;
    case ARG_MAX_Y:
      gtk_my_curve_set_range (my_curve, my_curve->min_x, my_curve->max_x,
                              my_curve->min_y, GTK_VALUE_FLOAT (*arg));
      break;
    }
}

static void
gtk_my_curve_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
  GtkMyCurve *my_curve = GTK_MY_CURVE (object);

  switch (arg_id)
    {
    case ARG_CURVE_TYPE:
      GTK_VALUE_ENUM (*arg) = my_curve->curve_type;
      break;
    case ARG_MIN_X:
      GTK_VALUE_FLOAT (*arg) = my_curve->min_x;
      break;
    case ARG_MAX_X:
      GTK_VALUE_FLOAT (*arg) = my_curve->max_x;
      break;
    case ARG_MIN_Y:
      GTK_VALUE_FLOAT (*arg) = my_curve->min_y;
      break;
    case ARG_MAX_Y:
      GTK_VALUE_FLOAT (*arg) = my_curve->max_y;
      break;
    default:
      arg->type = GTK_TYPE_INVALID;
      break;
    }
}

static int
project (gfloat value, gfloat min, gfloat max, int norm)
{
  return((norm - 1) * ((value - min) / (max - min)) + 0.5);
}

static gfloat
unproject (gint value, gfloat min, gfloat max, int norm)
{
  return(value / (gfloat) (norm - 1) * (max - min) + min);
}

/* Solve the tridiagonal equation system that determines the second
   derivatives for the interpolation points.  (Based on Numerical
   Recipies 2nd Edition.) */
static void
spline_solve (int n, gfloat x[], gfloat y[], gfloat y2[])
{
  gfloat p, sig, *u;
  gint i, k;

  u = g_malloc ((n - 1) * sizeof (u[0]));

  y2[0] = u[0] = 0.0;	/* set lower boundary condition to "natural" */

  for (i = 1; i < n - 1; ++i)
    {
      sig = (x[i] - x[i - 1]) / (x[i + 1] - x[i - 1]);
      p = sig * y2[i - 1] + 2.0;
      y2[i] = (sig - 1.0) / p;
      u[i] = ((y[i + 1] - y[i])
	      / (x[i + 1] - x[i]) - (y[i] - y[i - 1]) / (x[i] - x[i - 1]));
      u[i] = (6.0 * u[i] / (x[i + 1] - x[i - 1]) - sig * u[i - 1]) / p;
    }

  y2[n - 1] = 0.0;
  for (k = n - 2; k >= 0; --k)
    y2[k] = y2[k] * y2[k + 1] + u[k];

  g_free (u);
}

static gfloat
spline_eval (int n, gfloat x[], gfloat y[], gfloat y2[], gfloat val)
{
  gint k_lo, k_hi, k;
  gfloat h, b, a;

  /* do a binary search for the right interval: */
  k_lo = 0; k_hi = n - 1;
  while (k_hi - k_lo > 1)
    {
      k = (k_hi + k_lo) / 2;
      if (x[k] > val)
	k_hi = k;
      else
	k_lo = k;
    }

  h = x[k_hi] - x[k_lo];
  g_assert (h > 0.0);

  a = (x[k_hi] - val) / h;
  b = (val - x[k_lo]) / h;
  return a*y[k_lo] + b*y[k_hi] +
    ((a*a*a - a)*y2[k_lo] + (b*b*b - b)*y2[k_hi]) * (h*h)/6.0;
}

static void
gtk_my_curve_interpolate (GtkMyCurve *c, gint width, gint height)
{
  gfloat *vector;
  int i;

  vector =g_malloc (width * sizeof(vector[0]));

  gtk_my_curve_get_vector (c, width, vector);

  c->height = height;
  if (c->num_points != width)
    {
      c->num_points = width;
      if (c->point)
	g_free (c->point);
      c->point = g_malloc (c->num_points * sizeof (c->point[0]));
    }

  for (i = 0; i < width; ++i)
    {
      c->point[i].x = RADIUS + i;
      c->point[i].y = RADIUS + height
	- project (vector[i], c->min_y, c->max_y, height);
    }

  g_free (vector);
}

static void
gtk_my_curve_draw (GtkMyCurve *c, gint width, gint height)
{
  GtkStateType state;
  GtkStyle *style;
  gint i, x, y;

  if (!c->pixmap)
    return;

  if (c->height != height || c->num_points != width)
    gtk_my_curve_interpolate (c, width, height);

  state = GTK_STATE_NORMAL;
  if (!GTK_WIDGET_IS_SENSITIVE (GTK_WIDGET (c)))
    state = GTK_STATE_INSENSITIVE;

  style = GTK_WIDGET (c)->style;

  /* clear the pixmap: */
  gtk_paint_flat_box (style, c->pixmap, GTK_STATE_NORMAL, GTK_SHADOW_NONE,
		      NULL, GTK_WIDGET(c), "curve_bg",
		      0, 0, width + RADIUS * 2, height + RADIUS * 2);
  /* draw the grid lines: (XXX make more meaningful) */
  for (i = 0; i < 5; i++)
    {
      gdk_draw_line (c->pixmap, style->dark_gc[state],
		     RADIUS, i * (height / 4.0) + RADIUS,
		     width + RADIUS, i * (height / 4.0) + RADIUS);
      gdk_draw_line (c->pixmap, style->dark_gc[state],
		     i * (width / 4.0) + RADIUS, RADIUS,
		     i * (width / 4.0) + RADIUS, height + RADIUS);
    }

  gdk_draw_points (c->pixmap, style->fg_gc[state], c->point, c->num_points);
  if (c->curve_type != GTK_CURVE_TYPE_FREE)
    for (i = 0; i < c->num_ctlpoints; ++i)
      {
	if (c->ctlpoint[i][0] < c->min_x)
	  continue;

	x = project (c->ctlpoint[i][0], c->min_x, c->max_x,
		     width);
	y = height -
	  project (c->ctlpoint[i][1], c->min_y, c->max_y,
		   height);

	/* draw a bullet: */
	gdk_draw_arc (c->pixmap, style->fg_gc[state], TRUE, x, y,
		      RADIUS * 2, RADIUS*2, 0, 360*64);
      }
  gdk_draw_drawable (GTK_WIDGET (c)->window, style->fg_gc[state], c->pixmap,
		   0, 0, 0, 0, width + RADIUS * 2, height + RADIUS * 2);
}

static gint
gtk_my_curve_graph_events (GtkWidget *widget, GdkEvent *event, GtkMyCurve *c)
{
  GdkCursorType new_type = c->cursor_type;
  gint i, src, dst, leftbound, rightbound;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  GtkWidget *w;
  gint tx, ty;
  gint cx, x, y, width, height;
  gint closest_point = 0;
  gfloat rx, ry, min_x;
  guint distance;
  gint x1, x2, y1, y2;

  w = GTK_WIDGET (c);
  width = w->allocation.width - RADIUS * 2;
  height = w->allocation.height - RADIUS * 2;

  if ((width < 0) || (height < 0))
    return FALSE;

  /*  get the pointer position  */
  gdk_window_get_pointer (w->window, &tx, &ty, NULL);
  x = CLAMP ((tx - RADIUS), 0, width-1);
  y = CLAMP ((ty - RADIUS), 0, height-1);

  min_x = c->min_x;

  distance = ~0U;
  for (i = 0; i < c->num_ctlpoints; ++i)
    {
      cx = project (c->ctlpoint[i][0], min_x, c->max_x, width);
      if ((guint) abs (x - cx) < distance)
	{
	  distance = abs (x - cx);
	  closest_point = i;
	}
    }

  switch (event->type)
    {
    case GDK_CONFIGURE:
      if (c->pixmap)
	gdk_pixmap_unref (c->pixmap);
      c->pixmap = 0;
      /* fall through */
    case GDK_EXPOSE:
      if (!c->pixmap)
	c->pixmap = gdk_pixmap_new (w->window,
				    w->allocation.width,
				    w->allocation.height, -1);
      gtk_my_curve_draw (c, width, height);
      break;

    case GDK_BUTTON_PRESS:
      gtk_grab_add (widget);

      bevent = (GdkEventButton *) event;
      new_type = GDK_TCROSS;

      switch (c->curve_type)
	{
	case GTK_CURVE_TYPE_LINEAR:
	case GTK_CURVE_TYPE_SPLINE:
	  if (distance > MIN_DISTANCE)
	    {
	      /* insert a new control point */
	      if (c->num_ctlpoints > 0)
		{
		  cx = project (c->ctlpoint[closest_point][0], min_x,
				c->max_x, width);
		  if (x > cx)
		    ++closest_point;
		}
	      ++c->num_ctlpoints;
	      c->ctlpoint =
		g_realloc (c->ctlpoint,
			   c->num_ctlpoints * sizeof (*c->ctlpoint));
	      for (i = c->num_ctlpoints - 1; i > closest_point; --i)
		memcpy (c->ctlpoint + i, c->ctlpoint + i - 1,
			sizeof (*c->ctlpoint));
	    }
	  c->grab_point = closest_point;
	  c->ctlpoint[c->grab_point][0] =
	    unproject (x, min_x, c->max_x, width);
	  c->ctlpoint[c->grab_point][1] =
	    unproject (height - y, c->min_y, c->max_y, height);

	  gtk_my_curve_interpolate (c, width, height);
	  break;

	case GTK_CURVE_TYPE_FREE:
	  c->point[x].x = RADIUS + x;
	  c->point[x].y = RADIUS + y;
	  c->grab_point = x;
	  c->last = y;
	  break;
	}
      gtk_my_curve_draw (c, width, height);
      break;

    case GDK_BUTTON_RELEASE:
      gtk_grab_remove (widget);

      /* delete inactive points: */
      if (c->curve_type != GTK_CURVE_TYPE_FREE)
	{
	  for (src = dst = 0; src < c->num_ctlpoints; ++src)
	    {
	      if (c->ctlpoint[src][0] >= min_x)
		{
		  memcpy (c->ctlpoint + dst, c->ctlpoint + src,
			  sizeof (*c->ctlpoint));
		  ++dst;
		}
	    }
	  if (dst < src)
	    {
	      c->num_ctlpoints -= (src - dst);
	      if (c->num_ctlpoints <= 0)
		{
		  c->num_ctlpoints = 1;
		  c->ctlpoint[0][0] = min_x;
		  c->ctlpoint[0][1] = c->min_y;
		  gtk_my_curve_interpolate (c, width, height);
		  gtk_my_curve_draw (c, width, height);
		}
	      c->ctlpoint =
		g_realloc (c->ctlpoint,
			   c->num_ctlpoints * sizeof (*c->ctlpoint));
	    }
	}
      new_type = GDK_FLEUR;
      c->grab_point = -1;
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;

      switch (c->curve_type)
	{
	case GTK_CURVE_TYPE_LINEAR:
	case GTK_CURVE_TYPE_SPLINE:
	  if (c->grab_point == -1)
	    {
	      /* if no point is grabbed...  */
	      if (distance <= MIN_DISTANCE)
		new_type = GDK_FLEUR;
	      else
		new_type = GDK_TCROSS;
	    }
	  else
	    {
	      /* drag the grabbed point  */
	      new_type = GDK_TCROSS;

	      leftbound = -MIN_DISTANCE;
	      if (c->grab_point > 0)
		leftbound = project (c->ctlpoint[c->grab_point - 1][0],
				     min_x, c->max_x, width);

	      rightbound = width + RADIUS * 2 + MIN_DISTANCE;
	      if (c->grab_point + 1 < c->num_ctlpoints)
		rightbound = project (c->ctlpoint[c->grab_point + 1][0],
				      min_x, c->max_x, width);

	      if (tx <= leftbound || tx >= rightbound
		  || ty > height + RADIUS * 2 + MIN_DISTANCE
		  || ty < -MIN_DISTANCE)
		c->ctlpoint[c->grab_point][0] = min_x - 1.0;
	      else
		{
		  rx = unproject (x, min_x, c->max_x, width);
		  ry = unproject (height - y, c->min_y, c->max_y, height);
		  c->ctlpoint[c->grab_point][0] = rx;
		  c->ctlpoint[c->grab_point][1] = ry;
		}
	      gtk_my_curve_interpolate (c, width, height);
	      gtk_my_curve_draw (c, width, height);
	    }
	  break;

	case GTK_CURVE_TYPE_FREE:
	  if (c->grab_point != -1)
	    {
	      if (c->grab_point > x)
		{
		  x1 = x;
		  x2 = c->grab_point;
		  y1 = y;
		  y2 = c->last;
		}
	      else
		{
		  x1 = c->grab_point;
		  x2 = x;
		  y1 = c->last;
		  y2 = y;
		}

	      if (x2 != x1)
		for (i = x1; i <= x2; i++)
		  {
		    c->point[i].x = RADIUS + i;
		    c->point[i].y = RADIUS +
		      (y1 + ((y2 - y1) * (i - x1)) / (x2 - x1));
		  }
	      else
		{
		  c->point[x].x = RADIUS + x;
		  c->point[x].y = RADIUS + y;
		}
	      c->grab_point = x;
	      c->last = y;
	      gtk_my_curve_draw (c, width, height);
	    }
	  if (mevent->state & GDK_BUTTON1_MASK)
	    new_type = GDK_TCROSS;
	  else
	    new_type = GDK_PENCIL;
	  break;
	}
      if (new_type != (GdkCursorType) c->cursor_type)
	{
	  GdkCursor *cursor;

	  c->cursor_type = new_type;

	  cursor = gdk_cursor_new (c->cursor_type);
	  gdk_window_set_cursor (w->window, cursor);
	  gdk_cursor_unref (cursor);
	}
      break;

    default:
      break;
    }
  return FALSE;
}
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/
void
gtk_my_curve_set_curve_type (GtkMyCurve *c, GtkCurveType new_type)
{
  gfloat rx, dx;
  gint x, i;

  if (new_type != c->curve_type)
    {
      gint width, height;
      //HERE IS THE PROBLEM....when changing to CURVE_TYPE_LINEAR width=-5
      //and this produced a seg fault when memory allocation is attempted
      //to do in the  gtk_my_curve_interpolate (c, width, height)
      // with negative lenght...(!)
      //I've fixed it just taking the absolute value of the equation
      //it seems to work...but I am not sure it will produce another problem latter...
      width  = abs(GTK_WIDGET(c)->allocation.width - RADIUS * 2);
      //g_print(" \n width= %d RADIUS= %d  alloc.width= %d", width, RADIUS,GTK_WIDGET(c)->allocation.width);
      height = GTK_WIDGET(c)->allocation.height - RADIUS * 2;

      

      if (new_type == GTK_CURVE_TYPE_FREE)
	{
	  gtk_my_curve_interpolate (c, width, height);
	  c->curve_type = new_type;
	}
      else if (c->curve_type == GTK_CURVE_TYPE_FREE) //GTK_CURVE_TYPE_FREE
	{
	  if (c->ctlpoint)
	    g_free (c->ctlpoint);
	  c->num_ctlpoints = 9;

        
	  c->ctlpoint = g_malloc (c->num_ctlpoints * sizeof (*c->ctlpoint));

	  rx = 0.0;
	  dx = (width - 1) / (gfloat) (c->num_ctlpoints - 1);

	  for (i = 0; i < c->num_ctlpoints; ++i, rx += dx)
	    {
	      x = (int) (rx + 0.5);
	      c->ctlpoint[i][0] =
		unproject (x, c->min_x, c->max_x, width);
	      c->ctlpoint[i][1] =
		unproject (RADIUS + height - c->point[x].y,
			   c->min_y, c->max_y, height);
	    }
	  c->curve_type = new_type;
	  gtk_my_curve_interpolate (c, width, height);
	}
      else
	{
	  c->curve_type = new_type;
	  gtk_my_curve_interpolate (c, width, height);
	}
      g_signal_emit (G_OBJECT (c), curve_type_changed_signal, 0);
      gtk_my_curve_draw (c, width, height);
    }
}
/********************************************************************************************/
/********************************************************************************************/
/********************************************************************************************/

static void
gtk_my_curve_size_graph (GtkMyCurve *my_curve)
{
  gint width, height;
  gfloat aspect;

  width  = (my_curve->max_x - my_curve->min_x) + 1;
  height = (my_curve->max_y - my_curve->min_y) + 1;
  aspect = width / (gfloat) height;
  if (width > gdk_screen_width () / 4)
    width  = gdk_screen_width () / 4;
  if (height > gdk_screen_height () / 4)
    height = gdk_screen_height () / 4;

  if (aspect < 1.0)
    width  = height * aspect;
  else
    height = width / aspect;

  gtk_drawing_area_size (GTK_DRAWING_AREA (my_curve),
			 width + RADIUS * 2, height + RADIUS * 2);
}

static void
gtk_my_curve_reset_vector (GtkMyCurve *my_curve)
{
  if (my_curve->ctlpoint)
    g_free (my_curve->ctlpoint);

  my_curve->num_ctlpoints = 2;
  my_curve->ctlpoint = g_malloc (2 * sizeof (my_curve->ctlpoint[0]));
  my_curve->ctlpoint[0][0] = my_curve->min_x;
  my_curve->ctlpoint[0][1] = my_curve->min_y;
  my_curve->ctlpoint[1][0] = my_curve->max_x;
  my_curve->ctlpoint[1][1] = my_curve->max_y;

  if (my_curve->pixmap)
    {
      gint width, height;

      width = GTK_WIDGET (my_curve)->allocation.width - RADIUS * 2;
      height = GTK_WIDGET (my_curve)->allocation.height - RADIUS * 2;

      if (my_curve->curve_type == GTK_CURVE_TYPE_FREE)
	{
	  my_curve->curve_type = GTK_CURVE_TYPE_LINEAR;
	  gtk_my_curve_interpolate (my_curve, width, height);
	  my_curve->curve_type = GTK_CURVE_TYPE_FREE;
	}
      else
	gtk_my_curve_interpolate (my_curve, width, height);
      gtk_my_curve_draw (my_curve, width, height);
    }
}

void
gtk_my_curve_reset (GtkMyCurve *c)
{
  GtkCurveType old_type;

  old_type = c->curve_type;
  c->curve_type = GTK_CURVE_TYPE_SPLINE;
  gtk_my_curve_reset_vector (c);

  if (old_type != GTK_CURVE_TYPE_SPLINE)
    g_signal_emit (G_OBJECT (c), curve_type_changed_signal, 0);
}

void
gtk_my_curve_set_gamma (GtkMyCurve *c, gfloat gamma)
{
  gfloat x, one_over_gamma, height, one_over_width;
  GtkCurveType old_type;
  gint i;

  if (c->num_points < 2)
    return;

  old_type = c->curve_type;
  c->curve_type = GTK_CURVE_TYPE_FREE;

  if (gamma <= 0)
    one_over_gamma = 1.0;
  else
    one_over_gamma = 1.0 / gamma;
  one_over_width = 1.0 / (c->num_points - 1);
  height = c->height;
  for (i = 0; i < c->num_points; ++i)
    {
      x = (gfloat) i / (c->num_points - 1);
      c->point[i].x = RADIUS + i;
      c->point[i].y =
	RADIUS + (height * (1.0 - pow (x, one_over_gamma)) + 0.5);
    }

  if (old_type != GTK_CURVE_TYPE_FREE)
    g_signal_emit (G_OBJECT (c), curve_type_changed_signal, 0);

  gtk_my_curve_draw (c, c->num_points, c->height);
}

void
gtk_my_curve_set_range (GtkMyCurve *my_curve,
		     gfloat min_x, gfloat max_x, gfloat min_y, gfloat max_y)
{
  my_curve->min_x = min_x;
  my_curve->max_x = max_x;
  my_curve->min_y = min_y;
  my_curve->max_y = max_y;

  gtk_my_curve_size_graph (my_curve);
  gtk_my_curve_reset_vector (my_curve);
}
///////////////////////////////////////////////////////////////////////////////////////////////
void
gtk_my_curve_set_vector (GtkMyCurve *c, int veclen, gfloat vector[])
{
  GtkCurveType old_type;
  gfloat rx, dx, ry;
  gint i, height;

  old_type = c->curve_type;
  //c->curve_type = GTK_CURVE_TYPE_FREE;

  if (c->point)
    height = GTK_WIDGET (c)->allocation.height - RADIUS * 2;
  else
    {
      height = (c->max_y - c->min_y);
      if (height > gdk_screen_height () / 4)
	height = gdk_screen_height () / 4;

      c->height = height;
      c->num_points = veclen;
  	c->point = g_malloc (c->num_points * sizeof (c->point[0]));
    }
  rx = 0.;
  dx = ((gfloat)veclen - 1.0) / ((gfloat)c->num_points - 1.0);

  for (i = 0; i < c->num_points; ++i, rx += dx)
    {
      ry = vector[(int) (rx + 0.5)];
      if (ry > c->max_y) ry = c->max_y;
      if (ry < c->min_y) ry = c->min_y;
      c->point[i].x = RADIUS + i;
      c->point[i].y =
	RADIUS + height - project (ry, c->min_y, c->max_y, height);
    }
  //if (old_type != GTK_CURVE_TYPE_FREE)
  //g_signal_emit (G_OBJECT (c), curve_type_changed_signal, 0);

  gtk_my_curve_draw (c, c->num_points, height);
}
///////////////////////////////////////////////////////////////////////////////////////////////
void
gtk_my_curve_get_vector (GtkMyCurve *c, int veclen, gfloat vector[])
{
  gfloat rx, ry, dx, dy, min_x, delta_x, *mem, *xv, *yv, *y2v, prev;
  gint dst, i, x, next, num_active_ctlpoints = 0, first_active = -1;

  min_x = c->min_x;

  if (c->curve_type != GTK_CURVE_TYPE_FREE)
    {
      /* count active points: */
      prev = min_x - 1.0;
      for (i = num_active_ctlpoints = 0; i < c->num_ctlpoints; ++i)
	if (c->ctlpoint[i][0] > prev)
	  {
	    if (first_active < 0)
	      first_active = i;
	    prev = c->ctlpoint[i][0];
	    ++num_active_ctlpoints;
	  }

      /* handle degenerate case: */
      if (num_active_ctlpoints < 2)
	{
	  if (num_active_ctlpoints > 0)
	    ry = c->ctlpoint[first_active][1];
	  else
	    ry = c->min_y;
	  if (ry < c->min_y) ry = c->min_y;
	  if (ry > c->max_y) ry = c->max_y;
	  for (x = 0; x < veclen; ++x)
	    vector[x] = ry;
	  return;
	}
    }

  switch (c->curve_type)
    {
    case GTK_CURVE_TYPE_SPLINE:
      mem = g_malloc (3 * num_active_ctlpoints * sizeof (gfloat));
      xv  = mem;
      yv  = mem + num_active_ctlpoints;
      y2v = mem + 2*num_active_ctlpoints;

      prev = min_x - 1.0;
      for (i = dst = 0; i < c->num_ctlpoints; ++i)
	if (c->ctlpoint[i][0] > prev)
	  {
	    prev    = c->ctlpoint[i][0];
	    xv[dst] = c->ctlpoint[i][0];
	    yv[dst] = c->ctlpoint[i][1];
	    ++dst;
	  }

      spline_solve (num_active_ctlpoints, xv, yv, y2v);

      rx = min_x;
      dx = (c->max_x - min_x) / (veclen - 1);
      for (x = 0; x < veclen; ++x, rx += dx)
	{
	  ry = spline_eval (num_active_ctlpoints, xv, yv, y2v, rx);
	  if (ry < c->min_y) ry = c->min_y;
	  if (ry > c->max_y) ry = c->max_y;
	  vector[x] = ry;
	}

      g_free (mem);
      break;

    case GTK_CURVE_TYPE_LINEAR:
      dx = (c->max_x - min_x) / (veclen - 1);
      rx = min_x;
      ry = c->min_y;
      dy = 0.0;
      i  = first_active;
      for (x = 0; x < veclen; ++x, rx += dx)
	{
	  if (rx >= c->ctlpoint[i][0])
	    {
	      if (rx > c->ctlpoint[i][0])
		ry = c->min_y;
	      dy = 0.0;
	      next = i + 1;
	      while (next < c->num_ctlpoints
		     && c->ctlpoint[next][0] <= c->ctlpoint[i][0])
		++next;
	      if (next < c->num_ctlpoints)
		{
		  delta_x = c->ctlpoint[next][0] - c->ctlpoint[i][0];
		  dy = ((c->ctlpoint[next][1] - c->ctlpoint[i][1])
			/ delta_x);
		  dy *= dx;
		  ry = c->ctlpoint[i][1];
		  i = next;
		}
	    }
	  vector[x] = ry;
	  ry += dy;
	}
      break;

    case GTK_CURVE_TYPE_FREE:
      if (c->point)
	{
	  rx = 0.0;
	  dx = c->num_points / (double) veclen;
	  for (x = 0; x < veclen; ++x, rx += dx)
	    vector[x] = unproject (RADIUS + c->height - c->point[(int) rx].y,
				   c->min_y, c->max_y,
				   c->height);
	}
      else
	memset (vector, 0, veclen * sizeof (vector[0]));
      break;
    }
}

GtkWidget*
gtk_my_curve_new (void)
{
  return gtk_type_new (gtk_my_curve_get_type ());
}

// static void
// gtk_my_curve_finalize (GObject *object)
// {
//   GtkCurve *my_curve;

//   g_return_if_fail (object != NULL);
//   g_return_if_fail (GTK_IS_MY_CURVE (object));

//   my_curve = GTK_MY_CURVE (object);
//   if (my_curve->pixmap)
//     gdk_pixmap_unref (my_curve->pixmap);
//   if (my_curve->point)
//     g_free (my_curve->point);
//   if (my_curve->ctlpoint)
//     g_free (my_curve->ctlpoint);

//   G_OBJECT_CLASS (parent_class)->finalize (object);
// }
