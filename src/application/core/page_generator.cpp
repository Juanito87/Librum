#include "page_generator.hpp"
#include <cmath>
#include "fz_utils.hpp"
#include "mupdf/classes.h"
#include "mupdf/classes2.h"

namespace application::core
{

using utils::FzPointPair;

application::core::PageGenerator::PageGenerator(mupdf::FzDocument* document,
                                                int pageNumber) :
    m_document(document),
    m_textSelector(nullptr)
{
    m_page =
        std::make_unique<mupdf::FzPage>(m_document->fz_load_page(pageNumber));
    auto boundPage = m_page->fz_bound_page();

    setupDisplayList(boundPage);
    setupTextPage(pageNumber);
    setupSymbolBounds();
    setupLinks();

    m_textSelector = utils::TextSelector(m_textPage.get());
}

void PageGenerator::setupDisplayList(const mupdf::FzRect& boundPage)
{
    m_displayList = mupdf::FzDisplayList(boundPage);

    auto listDevice = m_displayList.fz_new_list_device();
    mupdf::FzCookie defaultCookie;
    m_page->fz_run_page(listDevice, mupdf::FzMatrix(), defaultCookie);
    listDevice.fz_close_device();
}

void PageGenerator::setupTextPage(int pageNumber)
{
    mupdf::FzStextOptions options;
    m_textPage =
        std::make_unique<mupdf::FzStextPage>(*m_document, pageNumber, options);
}

void PageGenerator::setupSymbolBounds()
{
    auto curr = m_textPage->begin();
    auto end = m_textPage->end();

    m_symbolBounds.resize(100);
    while(curr != end)
    {
        auto symbol = curr;
        fz_rect rect = symbol->m_internal->bbox;
        m_symbolBounds.emplace_back(rect);
        ++curr;
    }
}

void PageGenerator::setupLinks()
{
    auto end = m_page->fz_load_links().end();
    auto curr = m_page->fz_load_links().begin();
    while(curr != end)
    {
        m_links.push_back(*curr);
        ++curr;
    }
}

mupdf::FzPixmap PageGenerator::renderPage(float zoom)
{
    // Create matrix with zoom
    mupdf::FzMatrix matrix;
    matrix.a = zoom;
    matrix.d = zoom;

    auto pixmap = getEmptyPixmap(matrix);
    auto drawDevice = mupdf::fz_new_draw_device(mupdf::FzMatrix(), pixmap);

    mupdf::FzCookie cookie;
    mupdf::FzRect rect = mupdf::FzRect::Fixed_INFINITE;
    m_displayList.fz_run_display_list(drawDevice, matrix, rect, cookie);
    drawDevice.fz_close_device();

    if(m_invertColor)
        pixmap.fz_invert_pixmap();

    return pixmap;
}

mupdf::FzPixmap PageGenerator::getEmptyPixmap(
    const mupdf::FzMatrix& matrix) const
{
    auto bbox = m_page->fz_bound_page_box(FZ_CROP_BOX);
    auto scaledBbox = bbox.fz_transform_rect(matrix);

    mupdf::FzPixmap pixmap(mupdf::FzColorspace::Fixed_RGB, scaledBbox,
                           mupdf::FzSeparations(), 0);
    pixmap.fz_clear_pixmap();

    return pixmap;
}

void imageCleanupHandler(void* data)
{
    unsigned char* samples = static_cast<unsigned char*>(data);
    delete[] samples;
}

int PageGenerator::getWidth() const
{
    auto bbox = m_page->fz_bound_page_box(FZ_CROP_BOX);

    return (bbox.x1 - bbox.x0);
}

int PageGenerator::getHeight() const
{
    auto bbox = m_page->fz_bound_page_box(FZ_CROP_BOX);

    return (bbox.y1 - bbox.y0);
}

QList<mupdf::FzQuad>& PageGenerator::getBufferedSelectionRects()
{
    return m_bufferedSelectionRects;
}

void PageGenerator::setInvertColor(bool newInvertColor)
{
    if(m_invertColor == newInvertColor)
        return;

    m_invertColor = newInvertColor;
}

void PageGenerator::generateSelectionRects(mupdf::FzPoint start,
                                           mupdf::FzPoint end)
{
    m_textSelector.generateSelectionRects(m_bufferedSelectionRects, start, end);
}

FzPointPair PageGenerator::getPositionsForWordSelection(mupdf::FzPoint begin,
                                                        mupdf::FzPoint end)
{
    return m_textSelector.getPositionsForWordSelection(begin, end);
}

FzPointPair PageGenerator::getPositionsForLineSelection(mupdf::FzPoint point)
{
    return m_textSelector.getPositionsForLineSelection(point);
}

std::string PageGenerator::getTextFromSelection(mupdf::FzPoint start,
                                                mupdf::FzPoint end)
{
    return m_textSelector.getTextFromSelection(start, end);
}

bool PageGenerator::pointIsAboveText(const mupdf::FzPoint& point)
{
    mupdf::FzPoint fzPoint(point.x, point.y);
    for(auto& rect : m_symbolBounds)
    {
        if(fzPoint.fz_is_point_inside_rect(rect))
            return true;
    }

    return false;
}

bool PageGenerator::pointIsAboveLink(const mupdf::FzPoint& point)
{
    mupdf::FzPoint fzPoint(point.x, point.y);
    for(auto& link : m_links)
    {
        if(fzPoint.fz_is_point_inside_rect(link.rect()))
            return true;
    }

    return false;
}

mupdf::FzLink PageGenerator::getLinkAtPoint(const mupdf::FzPoint& point)
{
    mupdf::FzPoint fzPoint(point.x, point.y);
    for(auto& link : m_links)
    {
        if(fzPoint.fz_is_point_inside_rect(link.rect()))
            return link;
    }

    return mupdf::FzLink();
}

}  // namespace application::core