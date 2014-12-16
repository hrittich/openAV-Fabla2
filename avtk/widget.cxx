
#include "widget.hxx"

#include "ui.hxx"
#include "theme.hxx"

namespace Avtk
{

Widget::Widget( Avtk::UI* ui_, int x_, int y_, int w_, int h_, std::string label__) :
  ui(ui_),
  parent_( 0 ),
  theme_( ui->theme() ),
  x( x_ ),
  y( y_ ),
  w( w_ ),
  h( h_ ),
  label_( label__ ),
  visible_( true ),
  
  value_( 0 ),
  mouseButtonPressed_(0),
  callback( 0 ),
  callbackUD( 0 ),
  
  dm( DM_NONE ),
  mX(0),
  mY(0),
  scrollDisable( 0 ),
  scrollInvert( 0 )
{
}

void Widget::theme( Theme* t )
{
  theme_ = t;
}

int Widget::handle( const PuglEvent* event )
{
  switch (event->type)
  {
    
    case PUGL_BUTTON_PRESS:
      {
        if ( event->button.x == 0 && event->button.y == 0 )
          return 0;
        
        if( touches( event->button.x, event->button.y ) )
        {
          if( cm == CLICK_TOGGLE )
          {
            value( !value() );
            ui->redraw( this );
          }
          else if ( cm == CLICK_VALUE_FROM_Y )
          {
            float tmp = (event->button.y - y) / h/0.92;
            value( tmp );
#ifdef AVTK_DEBUG
            printf("value from Y, %f\n", tmp);
#endif // AVTK_DEBUG
            ui->redraw( this );
          }
          
          
          if( dm == DM_DRAG_VERTICAL ||
              dm == DM_DRAG_HORIZONTAL )
          {
            // sample the vertical mouse position, drag events affect += value()
            mX = event->button.x;
            mY = event->button.y;
          }
          
          // tell the UI that the current widget wants motion notify updates
          // this also handles Drag-n-Drop, so we need motion updates even if we
          // don't have DM_DRAG_VERTICAL || DM_DRAG_HORIZONTAL
          ui->wantsMotionUpdates( this, true );
          return 1;
        }
      }
      break;
    
    case PUGL_BUTTON_RELEASE:
      {
        ui->wantsMotionUpdates( this, false );
      }
      return 1;
      break;
    
    case PUGL_SCROLL:
      {
        bool scTch = touches( event->scroll.x, event->scroll.y );
        if( scTch )
        {
#ifdef AVTK_DEBUG
          printf("scroll touch %i, x %lf, y %lf\n", int(scTch), event->scroll.x, event->scroll.y );
#endif // AVTK_DEBUG
          float delta = event->scroll.dy / 10.f;
          if( scrollInvert )
            delta = -delta;
          value( value_ + delta );
          ui->redraw( this );
          return 1;
        }
      } break;
    
    case PUGL_KEY_PRESS:
      if (event->key.character == 'w')
      {
        /*
        printf("pugl key w\n");
        float delta = 1 / 10.f;
        value( value_ + delta );
        ui->redraw( this );
        return 1;
        */
      }
      else if (event->key.character == 's')
      {
        /*
        printf("pugl key s\n");
        float delta = 1 / 10.f;
        value( value_ - delta );
        ui->redraw( this );
        return 1;
        */
      }
      break;
    
    
    default:
      return 0; break;
  }
  
  return 0;
}

void Widget::motion( int x, int y )
{
  if ( dm == DM_NONE )
  {
    // if widget is pressed, and mouse moves outside the widget area
    // inform UI of possible drag-drop action
    if( !touches( x, y ) )
    {
      static const char* testData = "DragDropTestPayload";
#ifdef AVTK_DEBUG
      printf("motion outside widget -> DND?\n");
#endif // AVTK_DEBUG
      ui->dragDropInit( this, strlen( testData ), (void*)testData );
    }
    return;
  }
  
  // handle value() on the widget
  float delta = ( mY - y ) / float(h);
  
  if ( dm == DM_DRAG_HORIZONTAL )
    delta = ( x - mX ) / float(w);
  
  value( value_ + delta );
  //printf("drag(), delta %i, new value %\n", delta, value() );
  
  mX = x;
  mY = y;
  
  ui->redraw( this );
}

void Widget::value( float v )
{
  if( v > 1.0 ) v = 1.0;
  if( v < 0.0 ) v = 0.0;
  
  value_ = v;
  
  // call the callback if its set, and not told not to
  if ( callback )
    callback( this, callbackUD );
}

bool Widget::touches( int inx, int iny )
{
  return ( inx >= x && inx <= x + w && iny >= y && iny <= y + h);
}

void Widget::clickMode( ClickMode c, int cms )
{
  cm = c;
  clickModeSize = cms;
}

void Widget::visible( bool v )
{
  visible_ = v;
  ui->redraw( this );
}

void Widget::parent( Group* p )
{
  parent_ = p;
}

void Widget::dragMode( DragMode d )
{
  dm = d;
}

}; // Avtk
