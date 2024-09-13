#include "port.h"

#include "id.h"
#include "id_data.h"
#include "plot3d.h"
#include "rotate.h"

#include "win_defines.h"
#include <crtdbg.h>
#include <Windows.h>
#include <windowsx.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "plot.h"
#include "ods.h"

#define PLOT_TIMER_ID 1

static Plot *s_plot{};
static const char *const s_window_class{"IdPlot"};

static const BYTE font_8x8[8][1024/8] =
{
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x14, 0x14, 0x08, 0x00, 0x18, 0x08,
        0x04, 0x10, 0x08, 0x08, 0x00, 0x00, 0x00, 0x01,
        0x1C, 0x08, 0x1C, 0x1C, 0x0C, 0x3E, 0x1C, 0x3E,
        0x1C, 0x1C, 0x00, 0x00, 0x04, 0x00, 0x20, 0x1C,
        0x1C, 0x1C, 0x3C, 0x1C, 0x3C, 0x3E, 0x3E, 0x1C,
        0x22, 0x1C, 0x0E, 0x22, 0x10, 0x41, 0x22, 0x1C,
        0x1C, 0x1C, 0x3C, 0x1C, 0x3E, 0x22, 0x22, 0x41,
        0x22, 0x22, 0x3E, 0x1C, 0x40, 0x1C, 0x08, 0x00,
        0x10, 0x00, 0x10, 0x00, 0x02, 0x00, 0x0C, 0x00,
        0x20, 0x00, 0x00, 0x20, 0x18, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x0C, 0x08, 0x30, 0x00, 0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x14, 0x14, 0x1E, 0x32, 0x28, 0x08,
        0x08, 0x08, 0x49, 0x08, 0x00, 0x00, 0x00, 0x02,
        0x22, 0x18, 0x22, 0x22, 0x14, 0x20, 0x22, 0x02,
        0x22, 0x22, 0x0C, 0x0C, 0x08, 0x00, 0x10, 0x22,
        0x22, 0x22, 0x22, 0x22, 0x22, 0x20, 0x20, 0x22,
        0x22, 0x08, 0x04, 0x22, 0x10, 0x63, 0x32, 0x22,
        0x12, 0x22, 0x22, 0x22, 0x08, 0x22, 0x22, 0x41,
        0x22, 0x22, 0x02, 0x10, 0x20, 0x04, 0x14, 0x00,
        0x08, 0x1C, 0x10, 0x00, 0x02, 0x00, 0x12, 0x00,
        0x20, 0x08, 0x08, 0x20, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x10, 0x08, 0x08, 0x00, 0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x00, 0x7F, 0x28, 0x34, 0x10, 0x00,
        0x10, 0x04, 0x2A, 0x08, 0x00, 0x00, 0x00, 0x04,
        0x22, 0x08, 0x02, 0x02, 0x24, 0x20, 0x20, 0x04,
        0x22, 0x22, 0x0C, 0x0C, 0x10, 0x7F, 0x08, 0x02,
        0x2E, 0x22, 0x22, 0x20, 0x22, 0x20, 0x20, 0x20,
        0x22, 0x08, 0x04, 0x24, 0x10, 0x55, 0x2A, 0x22,
        0x12, 0x22, 0x22, 0x20, 0x08, 0x22, 0x22, 0x41,
        0x14, 0x14, 0x04, 0x10, 0x10, 0x04, 0x22, 0x00,
        0x00, 0x02, 0x1C, 0x1C, 0x0E, 0x1C, 0x10, 0x1D,
        0x2C, 0x00, 0x00, 0x24, 0x08, 0xB6, 0x2C, 0x1C,
        0x2C, 0x1A, 0x2C, 0x1C, 0x1C, 0x24, 0x22, 0x41,
        0x22, 0x12, 0x3C, 0x10, 0x08, 0x08, 0x30, 0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x00, 0x14, 0x1C, 0x08, 0x28, 0x00,
        0x10, 0x04, 0x1C, 0x7F, 0x00, 0x7F, 0x00, 0x08,
        0x2A, 0x08, 0x04, 0x0C, 0x3E, 0x3C, 0x3C, 0x08,
        0x1C, 0x1E, 0x00, 0x00, 0x20, 0x00, 0x04, 0x04,
        0x2A, 0x3E, 0x3C, 0x20, 0x22, 0x3C, 0x3E, 0x2E,
        0x3E, 0x08, 0x04, 0x38, 0x10, 0x49, 0x2A, 0x22,
        0x1C, 0x22, 0x3C, 0x1C, 0x08, 0x22, 0x14, 0x2A,
        0x08, 0x08, 0x08, 0x10, 0x08, 0x04, 0x00, 0x00,
        0x00, 0x1E, 0x12, 0x20, 0x12, 0x22, 0x38, 0x22,
        0x32, 0x08, 0x08, 0x28, 0x08, 0x49, 0x12, 0x22,
        0x12, 0x24, 0x30, 0x20, 0x08, 0x24, 0x22, 0x41,
        0x14, 0x12, 0x04, 0x20, 0x08, 0x04, 0x49, 0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x00, 0x7F, 0x0A, 0x16, 0x46, 0x00,
        0x10, 0x04, 0x2A, 0x08, 0x0C, 0x00, 0x00, 0x10,
        0x22, 0x08, 0x08, 0x02, 0x04, 0x02, 0x22, 0x10,
        0x22, 0x02, 0x0C, 0x0C, 0x10, 0x7F, 0x08, 0x08,
        0x2E, 0x22, 0x22, 0x20, 0x22, 0x20, 0x20, 0x22,
        0x22, 0x08, 0x24, 0x24, 0x10, 0x41, 0x26, 0x22,
        0x10, 0x22, 0x28, 0x02, 0x08, 0x22, 0x14, 0x2A,
        0x14, 0x08, 0x10, 0x10, 0x04, 0x04, 0x00, 0x00,
        0x00, 0x22, 0x12, 0x20, 0x12, 0x3E, 0x10, 0x22,
        0x22, 0x08, 0x08, 0x30, 0x08, 0x49, 0x12, 0x22,
        0x12, 0x24, 0x20, 0x18, 0x08, 0x24, 0x22, 0x49,
        0x08, 0x12, 0x08, 0x10, 0x08, 0x08, 0x06, 0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x14, 0x3C, 0x26, 0x44, 0x00,
        0x08, 0x08, 0x49, 0x08, 0x0C, 0x00, 0x0C, 0x20,
        0x22, 0x08, 0x10, 0x22, 0x04, 0x22, 0x22, 0x10,
        0x22, 0x22, 0x0C, 0x0C, 0x08, 0x00, 0x10, 0x00,
        0x20, 0x22, 0x22, 0x22, 0x22, 0x20, 0x20, 0x22,
        0x22, 0x08, 0x24, 0x22, 0x10, 0x41, 0x22, 0x22,
        0x10, 0x22, 0x24, 0x22, 0x08, 0x22, 0x08, 0x14,
        0x22, 0x08, 0x20, 0x10, 0x02, 0x04, 0x00, 0x00,
        0x00, 0x22, 0x12, 0x20, 0x12, 0x20, 0x10, 0x1E,
        0x22, 0x08, 0x08, 0x28, 0x08, 0x41, 0x12, 0x22,
        0x1C, 0x1C, 0x20, 0x04, 0x08, 0x24, 0x14, 0x55,
        0x14, 0x0E, 0x10, 0x10, 0x08, 0x08, 0x00, 0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x00, 0x14, 0x08, 0x00, 0x3A, 0x00,
        0x04, 0x10, 0x08, 0x08, 0x04, 0x00, 0x0C, 0x40,
        0x1C, 0x1C, 0x3E, 0x1C, 0x0E, 0x1C, 0x1C, 0x10,
        0x1C, 0x1C, 0x00, 0x04, 0x04, 0x00, 0x20, 0x08,
        0x1C, 0x22, 0x3C, 0x1C, 0x3C, 0x3E, 0x20, 0x1C,
        0x22, 0x1C, 0x18, 0x22, 0x1E, 0x41, 0x22, 0x1C,
        0x10, 0x1C, 0x22, 0x1C, 0x08, 0x1C, 0x08, 0x14,
        0x22, 0x08, 0x3E, 0x1C, 0x01, 0x1C, 0x00, 0x00,
        0x00, 0x1D, 0x2C, 0x1C, 0x0D, 0x1C, 0x10, 0x02,
        0x22, 0x08, 0x08, 0x24, 0x08, 0x41, 0x12, 0x1C,
        0x10, 0x04, 0x20, 0x38, 0x08, 0x1A, 0x08, 0x22,
        0x22, 0x02, 0x3C, 0x0C, 0x08, 0x30, 0x00, 0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C,
        0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x10, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};

void Plot::set_dirty_region(int xmin, int ymin, int xmax, int ymax)
{
    RECT *r = &m_dirty_region;

    _ASSERTE(xmin < xmax);
    _ASSERTE(ymin < ymax);
    _ASSERTE((r->left <= r->right) && (r->top <= r->bottom));
    if (r->left < 0)
    {
        r->left = xmin;
        r->right = xmax;
        r->top = ymin;
        r->bottom = ymax;
        m_dirty = true;
    }
    else
    {
        if (xmin < r->left)
        {
            r->left = xmin;
            m_dirty = true;
        }
        if (xmax > r->right)
        {
            r->right = xmax;
            m_dirty = true;
        }
        if (ymin < r->top)
        {
            r->top = ymin;
            m_dirty = true;
        }
        if (ymax > r->bottom)
        {
            r->bottom = ymax;
            m_dirty = true;
        }
    }
}

/* init_pixels
 *
 * Resize the pixel array to sxdots by sydots and initialize it to zero.
 * Any existing pixel array is freed.
 */
void Plot::init_pixels()
{
    m_pixels.clear();
    m_saved_pixels.clear();
    m_width = g_screen_x_dots;
    m_height = g_screen_y_dots;
    m_row_len = m_width * sizeof(BYTE);
    m_row_len = ((m_row_len + 3)/4)*4;
    m_pixels_len = m_row_len * m_height;
    _ASSERTE(m_pixels_len > 0);
    m_pixels.resize(m_pixels_len);
    std::memset(&m_pixels[0], 0, m_pixels_len);
    m_dirty = false;
    {
        RECT dirty_rect = { -1, -1, -1, -1 };
        m_dirty_region = dirty_rect;
    }
    {
        BITMAPINFOHEADER *h = &m_bmi.bmiHeader;

        h->biSize = sizeof(m_bmi.bmiHeader);
        h->biWidth = m_width;
        h->biHeight = m_height;
        h->biPlanes = 1;
        h->biBitCount = 8;
        h->biCompression = BI_RGB;
        h->biSizeImage = 0;
        h->biClrUsed = 256;
    }
}

static void plot_OnPaint(HWND window)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(window, &ps);
    RECT *r = &ps.rcPaint;
    int width = r->right - r->left;
    int height = r->bottom - r->top;

    _ASSERTE(width >= 0 && height >= 0);
    if (width > 0 && height > 0)
    {
        DWORD status;
        status = StretchDIBits(dc,
                               0, 0, s_plot->m_width, s_plot->m_height,
                               0, 0, s_plot->m_width, s_plot->m_height,
                               &s_plot->m_pixels[0], &s_plot->m_bmi, DIB_RGB_COLORS, SRCCOPY);
        _ASSERTE(status != GDI_ERROR);
    }
    EndPaint(window, &ps);
}

static LRESULT CALLBACK plot_proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    _ASSERTE(s_plot != nullptr);
    switch (message)
    {
    case WM_PAINT:
        HANDLE_WM_PAINT(window, wp, lp, plot_OnPaint);
        break;

    default:
        return DefWindowProc(window, message, wp, lp);
    }

    return 0;
}

int Plot::init(HINSTANCE instance, LPCSTR title)
{
    WNDCLASS  wc;
    int result;

    m_instance = instance;
    std::strcpy(m_title, title);

    result = GetClassInfoA(m_instance, s_window_class, &wc);
    if (!result)
    {
        wc.style = 0;
        wc.lpfnWndProc = plot_proc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = m_instance;
        wc.hIcon = nullptr;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszMenuName =  m_title;
        wc.lpszClassName = s_window_class;

        result = RegisterClassA(&wc);
    }

    return result;
}

void Plot::terminate()
{
    {
        HBITMAP rendering = (HBITMAP) SelectObject(m_memory_dc, (HGDIOBJ) m_backup);
        _ASSERTE(rendering == m_rendering);
    }
    DeleteObject(m_rendering);
    DeleteDC(m_memory_dc);
    DestroyWindow(m_window);
}

void Plot::create_backing_store()
{
    {
        HDC dc = GetDC(m_window);
        m_memory_dc = CreateCompatibleDC(dc);
        _ASSERTE(m_memory_dc);
        ReleaseDC(m_window, dc);
    }

    m_rendering = CreateCompatibleBitmap(m_memory_dc, m_width, m_height);
    _ASSERTE(m_rendering);
    m_backup = (HBITMAP) SelectObject(m_memory_dc, (HGDIOBJ) m_rendering);

    m_font = CreateFont(8, 8, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                          DEFAULT_PITCH | FF_MODERN, "Courier");
    _ASSERTE(m_font);
    SelectObject(m_memory_dc, (HGDIOBJ) m_font);
    SetBkMode(m_memory_dc, OPAQUE);
}

void Plot::create_window(HWND parent)
{
    if (nullptr == m_window)
    {
        init_pixels();
        s_plot = this;
        m_parent = parent;
        m_window = CreateWindowA(s_window_class,     //
            m_title,                                 //
            parent ? WS_CHILD : WS_OVERLAPPEDWINDOW, //
            CW_USEDEFAULT,                           // default horizontal position
            CW_USEDEFAULT,                           // default vertical position
            m_width, m_height,                       //
            parent,                                  //
            nullptr,                                 //
            m_instance,                              //
            nullptr);

        create_backing_store();
    }
}

void Plot::write_pixel(int x, int y, int color)
{
    _ASSERTE(m_pixels.size() == m_width* m_height);
    if (x < 0 || x > m_width || y < 0 || y > m_height)
    {
        return;
    }
    m_pixels[(m_height - y - 1)*m_row_len + x] = (BYTE)(color & 0xFF);
    set_dirty_region(x, y, x+1, y+1);
}

int Plot::read_pixel(int x, int y)
{
    _ASSERTE(m_pixels.size() == m_width* m_height);
    if (x < 0 || x > m_width || y < 0 || y > m_height)
    {
        return 0;
    }
    return (int) m_pixels[(m_height - 1 - y)*m_row_len + x];
}

void Plot::write_span(int y, int x, int lastx, const BYTE *pixels)
{
    int width = lastx-x+1;

    for (int i = 0; i < width; i++)
    {
        write_pixel(x+i, y, pixels[i]);
    }
    set_dirty_region(x, y, lastx+1, y+1);
}

void Plot::flush()
{
    if (m_dirty)
    {
        RECT r = { -1, -1, -1, -1 };
        InvalidateRect(m_window, nullptr, FALSE);
        m_dirty = false;
        m_dirty_region = r;
    }
}

void Plot::read_span(int y, int x, int lastx, BYTE *pixels)
{
    flush();
    int width = lastx - x + 1;
    for (int i = 0; i < width; i++)
    {
        pixels[i] = read_pixel(x + i, y);
    }
}

void Plot::set_line_mode(int mode)
{
}

void Plot::draw_line(int x1, int y1, int x2, int y2, int color)
{
    ::draw_line(x1, y1, x2, y2, color);
}

int Plot::resize()
{
    if ((g_screen_x_dots == m_width) && (g_screen_y_dots == m_height))
    {
        return 0;
    }

    init_pixels();
    BOOL status = SetWindowPos(m_window, nullptr, 0, 0, m_width, m_height, SWP_NOZORDER | SWP_NOMOVE);
    _ASSERTE(status);

    return 1;
}

int Plot::read_palette()
{
    if (!g_got_real_dac)
    {
        return -1;
    }

    for (int i = 0; i < 256; i++)
    {
        g_dac_box[i][0] = m_clut[i][0];
        g_dac_box[i][1] = m_clut[i][1];
        g_dac_box[i][2] = m_clut[i][2];
    }
    return 0;
}

int Plot::write_palette()
{
    for (int i = 0; i < 256; i++)
    {
        m_clut[i][0] = g_dac_box[i][0];
        m_clut[i][1] = g_dac_box[i][1];
        m_clut[i][2] = g_dac_box[i][2];

        m_bmi.bmiColors[i].rgbRed = g_dac_box[i][0]*4;
        m_bmi.bmiColors[i].rgbGreen = g_dac_box[i][1]*4;
        m_bmi.bmiColors[i].rgbBlue = g_dac_box[i][2]*4;
    }
    redraw();

    return 0;
}

static VOID CALLBACK redraw_window(HWND window, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
    InvalidateRect(window, nullptr, FALSE);
    KillTimer(window, PLOT_TIMER_ID);
}

void Plot::schedule_alarm(int secs)
{
    UINT_PTR result = SetTimer(m_window, PLOT_TIMER_ID, secs, redraw_window);
    if (!result)
    {
        DWORD error = GetLastError();
        _ASSERTE(result);
    }
}

void Plot::clear()
{
    RECT r = { 0, 0, m_width, m_height };
    m_dirty_region = r;
    m_dirty = true;
    std::memset(&m_pixels[0], 0, m_pixels_len);
}

void Plot::redraw()
{
    InvalidateRect(m_window, nullptr, FALSE);
}

void Plot::display_string(int x, int y, int fg, int bg, char const *text)
{
    while (*text)
    {
        for (int row = 0; row < 8; row++)
        {
            int x1 = x;
            int col = 8;
            BYTE pixel = font_8x8[row][static_cast<unsigned char>(*text)];
            while (col-- > 0)
            {
                int color = (pixel & (1 << col)) ? fg : bg;
                write_pixel(x1++, y + row, color);
            }
        }
        x += 8;
        text++;
    }
}

void Plot::save_graphics()
{
    m_saved_pixels = m_pixels;
}

void Plot::restore_graphics()
{
    m_pixels = m_saved_pixels;
    redraw();
}
