/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/XULResizerElement.h"
#include "mozilla/dom/XULResizerElementBinding.h"

#include "mozilla/EventDispatcher.h"
#include "mozilla/PresShell.h"
#include "mozilla/dom/Document.h"
#include "mozilla/dom/DocumentInlines.h"
#include "mozilla/MouseEvents.h"
#include "nsContentUtils.h"
#include "nsICSSDeclaration.h"
#include "nsIFrame.h"
#include "nsLayoutUtils.h"
#include "nsPresContext.h"
#include "nsStyledElement.h"

namespace mozilla::dom {

nsXULElement* NS_NewXULResizerElement(
    already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo) {
  RefPtr<mozilla::dom::NodeInfo> nodeInfo(aNodeInfo);
  auto* nim = nodeInfo->NodeInfoManager();
  return new (nim) XULResizerElement(nodeInfo.forget());
}

static bool GetEventPoint(const WidgetGUIEvent* aEvent,
                          LayoutDeviceIntPoint& aPoint) {
  NS_ENSURE_TRUE(aEvent, false);

  const WidgetTouchEvent* touchEvent = aEvent->AsTouchEvent();
  if (touchEvent) {
    // return false if there is more than one touch on the page, or if
    // we can't find a touch point
    if (touchEvent->mTouches.Length() != 1) {
      return false;
    }

    const dom::Touch* touch = touchEvent->mTouches.SafeElementAt(0);
    if (!touch) {
      return false;
    }
    aPoint = touch->mRefPoint;
  } else {
    aPoint = aEvent->mRefPoint;
  }
  return true;
}

JSObject* XULResizerElement::WrapNode(JSContext* aCx,
                                      JS::Handle<JSObject*> aGivenProto) {
  return XULResizerElement_Binding::Wrap(aCx, this, aGivenProto);
}

XULResizerElement::Direction XULResizerElement::GetDirection() {
  static const mozilla::dom::Element::AttrValuesArray strings[] = {
      // clang-format off
     nsGkAtoms::topleft,    nsGkAtoms::top,    nsGkAtoms::topright,
     nsGkAtoms::left,                          nsGkAtoms::right,
     nsGkAtoms::bottomleft, nsGkAtoms::bottom, nsGkAtoms::bottomright,
     nsGkAtoms::bottomstart,                   nsGkAtoms::bottomend,
     nullptr
      // clang-format on
  };

  static const Direction directions[] = {
      // clang-format off
     {-1, -1}, {0, -1}, {1, -1},
     {-1,  0},          {1,  0},
     {-1,  1}, {0,  1}, {1,  1},
     {-1,  1},          {1,  1}
      // clang-format on
  };

  const auto* frame = GetPrimaryFrame();
  if (!frame) {
    return directions[0];  // default: topleft
  }

  int32_t index =
      FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::dir, strings, eCaseMatters);
  if (index < 0) {
    return directions[0];  // default: topleft
  }

  if (index >= 8) {
    // Directions 8 and higher are RTL-aware directions and should reverse the
    // horizontal component if RTL.
    auto wm = frame->GetWritingMode();
    if (wm.IsPhysicalRTL()) {
      Direction direction = directions[index];
      direction.mHorizontal *= -1;
      return direction;
    }
  }

  return directions[index];
}

nsresult XULResizerElement::PostHandleEvent(EventChainPostVisitor& aVisitor) {
  if (aVisitor.mEventStatus != nsEventStatus_eConsumeNoDefault) {
    PostHandleEventInternal(aVisitor);
  }
  return nsXULElement::PostHandleEvent(aVisitor);
}

void XULResizerElement::PostHandleEventInternal(
    EventChainPostVisitor& aVisitor) {
  bool doDefault = true;
  const WidgetEvent& event = *aVisitor.mEvent;
  switch (event.mMessage) {
    case eTouchStart:
    case eMouseDown: {
      if (event.mClass == eTouchEventClass ||
          (event.mClass == eMouseEventClass &&
           event.AsMouseEvent()->mButton == MouseButton::ePrimary)) {
        nsIContent* contentToResize = GetContentToResize();
        if (!contentToResize) {
          break;  // don't do anything if there's nothing to resize
        }
        auto* guiEvent = event.AsGUIEvent();
        nsIFrame* frame = contentToResize->GetPrimaryFrame();
        if (!frame) {
          break;
        }
        // cache the content rectangle for the frame to resize
        mMouseDownSize =
            frame->StylePosition()->mBoxSizing == StyleBoxSizing::Content
                ? frame->GetContentRect().Size()
                : frame->GetRect().Size();

        // remember current mouse coordinates
        if (!GetEventPoint(guiEvent, mMouseDownPoint)) {
          break;
        }
        mTrackingMouseMove = true;
        PresShell::SetCapturingContent(this, CaptureFlags::IgnoreAllowedState);
        doDefault = false;
      }
    } break;

    case eTouchMove:
    case eMouseMove: {
      if (mTrackingMouseMove) {
        nsCOMPtr<nsIContent> contentToResize = GetContentToResize();
        if (!contentToResize) {
          break;  // don't do anything if there's nothing to resize
        }
        nsIFrame* frame = contentToResize->GetPrimaryFrame();
        if (!frame) {
          break;
        }

        // both MouseMove and direction are negative when pointing to the
        // top and left, and positive when pointing to the bottom and right

        // retrieve the offset of the mousemove event relative to the mousedown.
        // The difference is how much the resize needs to be
        LayoutDeviceIntPoint refPoint;
        auto* guiEvent = event.AsGUIEvent();
        if (!GetEventPoint(guiEvent, refPoint)) {
          break;
        }

        const nsPoint oldPos = nsLayoutUtils::GetEventCoordinatesRelativeTo(
            guiEvent->mWidget, mMouseDownPoint, RelativeTo{frame});
        const nsPoint newPos = nsLayoutUtils::GetEventCoordinatesRelativeTo(
            guiEvent->mWidget, refPoint, RelativeTo{frame});

        nsPoint mouseMove(newPos - oldPos);

        // Determine which direction to resize by checking the dir attribute.
        // For windows and menus, ensure that it can be resized in that
        // direction.
        Direction direction = GetDirection();

        nsSize newSize = mMouseDownSize;

        // Check if there are any size constraints on this window.
        newSize.width += direction.mHorizontal * mouseMove.x;
        newSize.height += direction.mVertical * mouseMove.y;

        if (newSize.width < AppUnitsPerCSSPixel() && mouseMove.x != 0) {
          newSize.width = AppUnitsPerCSSPixel();
        }

        if (newSize.height < AppUnitsPerCSSPixel() && mouseMove.y != 0) {
          newSize.height = AppUnitsPerCSSPixel();
        }

        // convert the rectangle into css pixels. When changing the size in a
        // direction, don't allow the new size to be less that the resizer's
        // size. This ensures that content isn't resized too small as to make
        // the resizer invisible.
        if (auto* resizerFrame = GetPrimaryFrame()) {
          nsRect resizerRect = resizerFrame->GetRect();
          if (newSize.width < resizerRect.width && mouseMove.x != 0) {
            newSize.width = resizerRect.width;
          }
          if (newSize.height < resizerRect.height && mouseMove.y != 0) {
            newSize.height = resizerRect.height;
          }
        }

        auto cssSize = CSSIntSize::FromAppUnitsRounded(newSize);

        SizeInfo sizeInfo, originalSizeInfo;
        sizeInfo.width.AppendInt(cssSize.width);
        sizeInfo.height.AppendInt(cssSize.height);
        ResizeContent(contentToResize, direction, sizeInfo, &originalSizeInfo);
        MaybePersistOriginalSize(contentToResize, originalSizeInfo);

        doDefault = false;
      }
    } break;

    case eMouseClick: {
      auto* mouseEvent = event.AsMouseEvent();
      if (mouseEvent->IsLeftClickEvent()) {
        // Execute the oncommand event handler.
        nsContentUtils::DispatchXULCommand(
            this, false, nullptr, nullptr, mouseEvent->IsControl(),
            mouseEvent->IsAlt(), mouseEvent->IsShift(), mouseEvent->IsMeta(),
            mouseEvent->mInputSource, mouseEvent->mButton);
      }
    } break;

    case eTouchEnd:
    case eMouseUp: {
      if (event.mClass == eTouchEventClass ||
          (event.mClass == eMouseEventClass &&
           event.AsMouseEvent()->mButton == MouseButton::ePrimary)) {
        mTrackingMouseMove = false;
        PresShell::ReleaseCapturingContent();
        doDefault = false;
      }
    } break;

    case eMouseDoubleClick: {
      if (event.AsMouseEvent()->mButton == MouseButton::ePrimary) {
        if (nsIContent* contentToResize = GetContentToResize()) {
          RestoreOriginalSize(contentToResize);
        }
      }
    } break;

    default:
      break;
  }

  if (!doDefault) {
    aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
  }
}

nsIContent* XULResizerElement::GetContentToResize() const {
  if (!IsInComposedDoc()) {
    return nullptr;
  }
  // Return the parent, but skip over native anonymous content
  nsIContent* parent = GetParent();
  return parent ? parent->FindFirstNonChromeOnlyAccessContent() : nullptr;
}

/* static */
void XULResizerElement::ResizeContent(nsIContent* aContent,
                                      const Direction& aDirection,
                                      const SizeInfo& aSizeInfo,
                                      SizeInfo* aOriginalSizeInfo) {
  RefPtr inlineStyleContent = nsStyledElement::FromNode(aContent);
  if (!inlineStyleContent) {
    return;
  }
  nsCOMPtr<nsICSSDeclaration> decl = inlineStyleContent->Style();
  if (aOriginalSizeInfo) {
    decl->GetPropertyValue("width"_ns, aOriginalSizeInfo->width);
    decl->GetPropertyValue("height"_ns, aOriginalSizeInfo->height);
  }

  // only set the property if the element could have changed in that
  // direction
  if (aDirection.mHorizontal) {
    nsAutoCString widthstr(aSizeInfo.width);
    if (!widthstr.IsEmpty() && !StringEndsWith(widthstr, "px"_ns)) {
      widthstr.AppendLiteral("px");
    }
    decl->SetProperty("width"_ns, widthstr, ""_ns, IgnoreErrors());
  }

  if (aDirection.mVertical) {
    nsAutoCString heightstr(aSizeInfo.height);
    if (!heightstr.IsEmpty() && !StringEndsWith(heightstr, "px"_ns)) {
      heightstr.AppendLiteral("px");
    }
    decl->SetProperty("height"_ns, heightstr, ""_ns, IgnoreErrors());
  }
}

/* static */
void XULResizerElement::MaybePersistOriginalSize(nsIContent* aContent,
                                                 const SizeInfo& aSizeInfo) {
  nsresult rv;
  aContent->GetProperty(nsGkAtoms::_moz_original_size, &rv);
  if (rv != NS_PROPTABLE_PROP_NOT_THERE) {
    return;
  }

  UniquePtr<SizeInfo> sizeInfo(new SizeInfo(aSizeInfo));
  rv = aContent->SetProperty(
      nsGkAtoms::_moz_original_size, sizeInfo.get(),
      nsINode::DeleteProperty<XULResizerElement::SizeInfo>);
  if (NS_SUCCEEDED(rv)) {
    Unused << sizeInfo.release();
  }
}

/* static */
void XULResizerElement::RestoreOriginalSize(nsIContent* aContent) {
  nsresult rv;
  SizeInfo* sizeInfo = static_cast<SizeInfo*>(
      aContent->GetProperty(nsGkAtoms::_moz_original_size, &rv));
  if (NS_FAILED(rv)) {
    return;
  }

  NS_ASSERTION(sizeInfo, "We set a null sizeInfo!?");
  Direction direction = {1, 1};
  ResizeContent(aContent, direction, *sizeInfo, nullptr);
  aContent->RemoveProperty(nsGkAtoms::_moz_original_size);
}

}  // namespace mozilla::dom
