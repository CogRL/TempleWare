// used: [stl] vector
#include <vector>
// used: [stl] sort
#include <algorithm>

#include "preview.h"

#include <cstddef> 
#include <xcharconv.h>
#include <xiosbase>
#include <iostream>

#include "..\..\..\utils\render\ui\menu.h"
#include "..\..\..\utils\render\render.h"
#include "../../../utils/configs/config.h"
#include <examples/example_win32_directx11/font.h>

#include <imgui.h>
#include <imgui_internal.h>
#include "../../../helpers/crt/crt.h"

/* asphaxcya esp builder slightly modified i want to import later on a real model in order to be interactive */

using namespace visuals;
float mAlpha[65];
#pragma region visual_overlay_components

ImVec2 preview::CBaseComponent::GetBasePosition(const ImVec4& box) const
{
	return { box[this->nSide == SIDE_RIGHT ? SIDE_RIGHT : SIDE_LEFT], box[this->nSide == SIDE_BOTTOM ? SIDE_BOTTOM : SIDE_TOP] };
}

ImVec2 preview::CBaseDirectionalComponent::GetBasePosition(const ImVec4& box) const
{
	ImVec2 vecBasePosition = {};

	if (this->nSide == SIDE_TOP || this->nSide == SIDE_BOTTOM)
	{
		vecBasePosition = { (box[SIDE_LEFT] + box[SIDE_RIGHT]) * 0.5f, box[this->nSide] };
	}
	else if (this->nSide == SIDE_LEFT || this->nSide == SIDE_RIGHT)
	{
		vecBasePosition = { box[this->nSide], box[this->nDirection == DIR_TOP ? SIDE_BOTTOM : SIDE_TOP] };
	}
	else
	{
		return vecBasePosition;
	}

	if (this->nSide != SIDE_RIGHT && this->nDirection != DIR_RIGHT)
		vecBasePosition.x -= this->vecSize.x * ((static_cast<std::uint8_t>(this->nDirection) == static_cast<std::uint8_t>(this->nSide) && (this->nSide & 1U) == 1U) ? 0.5f : 1.0f);

	if (this->nSide == SIDE_TOP || this->nDirection == DIR_TOP)
		vecBasePosition.y -= this->vecSize.y;

	return vecBasePosition;
}

preview::CBarComponent::CBarComponent(const bool bIsMenuItem, const EAlignSide nAlignSide, const ImVec4& vecBox, const float max_limit, const float flProgressFactor, const std::size_t uOverlayVarIndex, const float alphaM) :
	bIsMenuItem(bIsMenuItem), uOverlayVarIndex(uOverlayVarIndex), max_limit(math::Clamp(flProgressFactor, 0.f, 1.f)), flProgressFactor(math::Clamp(flProgressFactor, 0.f, 1.f)), alphaMultiplier(alphaM)
{
	this->nSide = nAlignSide;
	const bool bIsHorizontal = ((nAlignSide & 1U) == 1U);
	this->value = static_cast<int>(flProgressFactor * 100);
	this->alphaMultiplier = alphaM;
	if (this->nSide == SIDE_BOTTOM || this->nSide == SIDE_TOP)
		this->value_sz = std::to_string(static_cast<int>(flProgressFactor * max_limit)) + (" ");
	else
		this->value_sz = std::to_string(static_cast<int>(flProgressFactor * 100)) + (" ");

	const BarOverlayVar_t& overlayConfig = cfg_get( BarOverlayVar_t, uOverlayVarIndex);
	this->vecSize = { (bIsHorizontal ? vecBox[SIDE_RIGHT] - vecBox[SIDE_LEFT] : overlayConfig.flThickness), (bIsHorizontal ? overlayConfig.flThickness : vecBox[SIDE_BOTTOM] - vecBox[SIDE_TOP]) };
}

void preview::CBarComponent::Render(ImDrawList* pDrawList, const ImVec2& vecPosition)
{
	BarOverlayVar_t& overlayConfig = cfg_get(BarOverlayVar_t, uOverlayVarIndex);
	const ImVec2 vecThicknessOffset = { overlayConfig.flThickness, overlayConfig.flThickness };
	ImVec2 vecMin = vecPosition, vecMax = vecPosition + this->vecSize;
	overlayConfig.colShadow.SetAlphaM(this->alphaMultiplier);


	if (this->nSide == SIDE_BOTTOM || this->nSide == SIDE_TOP) {
		// bar glow shadow
		if (overlayConfig.bGlowShadow) {
			Color_t shadow_effect_color = overlayConfig.bUseFactorColor ? Color_t::FromHSB((flProgressFactor * 120.f) / 360.f, 1.0f, 0.6f * this->alphaMultiplier) : overlayConfig.colShadow;
		
		}
		// background glow
	
		// outline
		if (overlayConfig.bOutline)
			pDrawList->AddRect(ImVec2(vecMin.x + 2.f, vecMin.y), ImVec2(vecMax.x - 2.f, vecMax.y), overlayConfig.colOutline.GetU32(this->alphaMultiplier), 0.f,  overlayConfig.flThickness);
	}
	else {
		// bar glow shadow
		if (overlayConfig.bGlowShadow) {
			Color_t shadow_effect_color = overlayConfig.bUseFactorColor ? Color_t::FromHSB((flProgressFactor * 120.f) / 360.f, 1.0f, 0.6f * this->alphaMultiplier) : overlayConfig.colShadow;
		
		}

		// background glow
		
		// outline
		if (overlayConfig.bOutline)
			pDrawList->AddRect(ImVec2(vecMin.x, vecMin.y + 2.f), ImVec2(vecMax.x, vecMax.y - 2.f), overlayConfig.colOutline.GetU32(this->alphaMultiplier), 0.f,  overlayConfig.flThickness);

	}

	// account outline offset
	vecMin += vecThicknessOffset;
	vecMax -= vecThicknessOffset;

	const ImVec2 vecLineSize = vecMax - vecMin;
	float flPrevProgressFactor = flProgressFactor; // Store the previous progress factor
	flProgressFactor = std::clamp(flProgressFactor, 0.0f, 1.0f); // Ensure progress factor is within the valid range (0 to 1)

	// Apply animation smoothing
	const float flAnimationSpeed = 8.f; // Adjust as needed
	flPrevProgressFactor = std::lerp(flPrevProgressFactor, flProgressFactor, flAnimationSpeed * sdk::m_global_vars->m_absolute_frame_time);

	// Modify active side axis by factor
	if ((this->nSide & 1U) == 0U)
		vecMin.y += vecLineSize.y * (1.0f - flPrevProgressFactor);
	else
		vecMax.x -= vecLineSize.x * (1.0f - flPrevProgressFactor);

	if (overlayConfig.bGradient && !overlayConfig.bUseFactorColor)
	{
		if (this->nSide == SIDE_LEFT || this->nSide == SIDE_RIGHT)
			pDrawList->AddRectFilledMultiColor(vecMin, vecMax, overlayConfig.colPrimary.GetU32(this->alphaMultiplier), overlayConfig.colPrimary.GetU32(this->alphaMultiplier), overlayConfig.colSecondary.GetU32(this->alphaMultiplier), overlayConfig.colSecondary.GetU32(this->alphaMultiplier));
		else
			pDrawList->AddRectFilledMultiColor(vecMin, vecMax, overlayConfig.colSecondary.GetU32(this->alphaMultiplier), overlayConfig.colPrimary.GetU32(this->alphaMultiplier), overlayConfig.colPrimary.GetU32(this->alphaMultiplier), overlayConfig.colSecondary.GetU32(this->alphaMultiplier));
	}
	else
	{
		const ImU32 u32Color = overlayConfig.bUseFactorColor ? Color_t::FromHSB((flProgressFactor * 120.f) / 360.f, 1.0f, 1.0f).GetU32(this->alphaMultiplier) : overlayConfig.colPrimary.GetU32(this->alphaMultiplier);
		pDrawList->AddRectFilled(vecMin, vecMax, u32Color, 0.f);
	}


	std::string sz = this->value_sz;

	// Render text on the bar with black outline
	if (!sz.empty() && overlayConfig.bShowValue && this->value <= 92.f) {
		ImVec2 textSize = ImGui::CalcTextSize(sz.c_str());
		ImVec2 textPos = ImVec2((vecMin.x + vecMax.x - textSize.x) * 0.5f, (vecMin.y + vecMax.y - textSize.y) * 0.5f);

		// modify active side axis by factor
		int fill = this->nSide == SIDE_BOTTOM ? (int)std::round(flPrevProgressFactor * vecLineSize.y / this->max_limit) : (int)std::round(flPrevProgressFactor * vecLineSize.y / 100.f);

		if (this->nSide == SIDE_BOTTOM || this->nSide == SIDE_TOP) {
			textPos.y = vecMin.y - 7;
			textPos.x = vecMin.x + (fill)-5;
		}
		else {
			textPos.y = vecMin.y + (fill)-7;
			textPos.x = vecMin.x - 5;
		}
		// Draw the black outline first
		const int outlineThickness = 1; // Adjust as needed
		pDrawList->AddText(ImVec2(textPos.x - outlineThickness, textPos.y), IM_COL32(0, 0, 0, 255 * this->alphaMultiplier), sz.c_str());
		pDrawList->AddText( ImVec2(textPos.x + outlineThickness, textPos.y), IM_COL32(0, 0, 0, 255 * this->alphaMultiplier), sz.c_str());
		pDrawList->AddText( ImVec2(textPos.x, textPos.y - outlineThickness), IM_COL32(0, 0, 0, 255 * this->alphaMultiplier), sz.c_str());
		pDrawList->AddText( ImVec2(textPos.x, textPos.y + outlineThickness), IM_COL32(0, 0, 0, 255 * this->alphaMultiplier), sz.c_str());

		// Draw the original text on top
		pDrawList->AddText( textPos, IM_COL32(255, 255, 255, 255 * this->alphaMultiplier), sz.c_str());
	}

	// only open menu item if menu is opened and overlay is enabled
	bIsMenuItem &= (menu::m_opened && overlayConfig.bEnable);
	if (bIsMenuItem)
	{
		// @note: padding 2.f incase the thickness is too small
		this->bIsHovered = ImRect(vecPosition - ImVec2(2.f, 2.f), vecPosition + this->vecSize + ImVec2(2.f, 2.f)).Contains(ImGui::GetIO().MousePos);
		// if component is hovered + right clicked
		if (this->bIsHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			ImGui::OpenPopup((std::to_string(this->uOverlayVarIndex).c_str()));

		if (ImGui::BeginPopup((std::to_string(this->uOverlayVarIndex).c_str()), ImGuiWindowFlags_NoResize))
		{
			ImVec2 size = ImVec2(640, 500);
			ImGui::SetWindowSize(size);


		
		}
	}
	else
		// dont process hovered on menu close...
		this->bIsHovered = false;
}

preview::CTextComponent::CTextComponent(const bool bIsMenuItem, const bool bIcons, const EAlignSide nAlignSide, const EAlignDirection nAlignDirection, const ImFont* pFont, const char* szText, const std::size_t uOverlayVarIndex, const float alphaM) :
	bIsMenuItem(bIsMenuItem), bIcon(bIcons), pFont(pFont), uOverlayVarIndex(uOverlayVarIndex), alphaMultiplier(alphaM)
{
	const TextOverlayVar_t& overlayConfig = cfg_get(TextOverlayVar_t, uOverlayVarIndex);
	// allocate own buffer to safely store a copy of the string
	this->bIcon = bIcons;
	this->alphaMultiplier = alphaM;
	this->szText = new char[c_run_time::StringLength(szText) + 1U];
	c_run_time::StringCopy(this->szText, szText);
	this->nSide = nAlignSide;
	this->nDirection = nAlignDirection;
	this->vecSize = pFont->CalcTextSizeA(pFont->FontSize, FLT_MAX, 0.0f, szText) + overlayConfig.flThickness;
}

preview::CTextComponent::~CTextComponent()
{
	// deallocate buffer of the copied string
	delete[] this->szText;
}

void preview::CTextComponent::Render(ImDrawList* pDrawList, const ImVec2& vecPosition)
{
	TextOverlayVar_t& overlayConfig = cfg_get(TextOverlayVar_t, this->uOverlayVarIndex);

	const ImVec2 vecOutlineOffset = { overlayConfig.flThickness, overlayConfig.flThickness };


	// Adjust the text position based on the side
	ImVec2 textPos = vecPosition;

	// Adjust the text position based on the side
	if (this->nSide == SIDE_BOTTOM) {
		// Move the text 2 pixels below for SIDE_BOTTOM
		textPos.y += 2.0f;
	}
	else if (this->nSide == SIDE_TOP) {
		// Move the text 2 pixels above for SIDE_TOP
		textPos.y -= 1.0f;
	}

	// @test: used for spacing debugging
	//pDrawList->AddRect(textPos, textPos + this->vecSize, IM_COL32(255, 255, 255, 255));
	// @todo: fix this cringe shit after gui merge
	if (overlayConfig.flThickness >= 1.0f)
	{
		// Adjust the text position by 2 pixels above
		pDrawList->AddText(this->pFont, this->pFont->FontSize, textPos, overlayConfig.colOutline.GetU32(this->alphaMultiplier), this->szText);
		pDrawList->AddText(this->pFont, this->pFont->FontSize, textPos + vecOutlineOffset * 2.0f, overlayConfig.colOutline.GetU32(this->alphaMultiplier), this->szText);
	}

	// Adjust the text position by 2 pixels above
	pDrawList->AddText(this->pFont, this->pFont->FontSize, textPos + vecOutlineOffset, overlayConfig.colPrimary.GetU32(this->alphaMultiplier), this->szText);

	// only open menu item if menu is opened and overlay is enabled
	bIsMenuItem &= menu::m_opened && overlayConfig.bEnable;
	if (bIsMenuItem)
	{
		this->bIsHovered = ImRect(vecPosition, vecPosition + this->vecSize).Contains(ImGui::GetIO().MousePos);
		// @test: used for spacing debugging
		//pDrawList->AddRect(vecPosition, vecPosition + this->vecSize, IM_COL32(this->bIsHovered ? 0 : 255, this->bIsHovered ? 255 : 0, 0, 255));

		// if component is hovered + right clicked
		if (this->bIsHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			ImGui::OpenPopup((this->szText));

		if (ImGui::BeginPopup((this->szText), ImGuiWindowFlags_NoResize ))
		{

			ImVec2 size = ImVec2(615, 90);
			ImGui::SetWindowSize(size); // Adjust the size as needed
			

			ImGui::EndPopup();
		}
	}
}

#pragma endregion

#pragma region visual_overlay_context

bool preview::Context_t::AddBoxComponent(ImDrawList* pDrawList, const ImVec4& vecBox, const int nType, float flThickness, float flRounding, const Color_t& colPrimary, const Color_t& colOutline, const float AlphaMultiplier)
{
	flThickness = std::floorf(flThickness);
	const ImVec2 vecThicknessOffset = { flThickness, flThickness };

	switch (nType)
	{
	case VISUAL_OVERLAY_BOX_FULL:
	{
		const ImVec2 vecBoxMin = { vecBox[SIDE_LEFT], vecBox[SIDE_TOP] };
		const ImVec2 vecBoxMax = { vecBox[SIDE_RIGHT], vecBox[SIDE_BOTTOM] };

		// inner outline
		pDrawList->AddRect(vecBoxMin + vecThicknessOffset * 2.0f, vecBoxMax - vecThicknessOffset * 2.0f, colOutline.GetU32(AlphaMultiplier), flRounding, flThickness);
		// primary box
		pDrawList->AddRect(vecBoxMin + vecThicknessOffset, vecBoxMax - vecThicknessOffset, colPrimary.GetU32(AlphaMultiplier), flRounding, flThickness);
		// outer outline
		pDrawList->AddRect(vecBoxMin, vecBoxMax, colOutline.GetU32(AlphaMultiplier), flRounding,  flThickness);

		break;
	}
	case VISUAL_OVERLAY_BOX_CORNERS:
	{
		// corner part of the whole line
		constexpr float flPartRatio = 0.25f;

		const float flCornerWidth = ((vecBox[SIDE_RIGHT] - vecBox[SIDE_LEFT]) * flPartRatio);
		const float flCornerHeight = ((vecBox[SIDE_BOTTOM] - vecBox[SIDE_TOP]) * flPartRatio);

		const ImVec2 arrCornerPoints[4][3] = {
			// top-left
			{ ImVec2(vecBox[SIDE_LEFT], vecBox[SIDE_TOP] + flCornerHeight) + vecThicknessOffset, ImVec2(vecBox[SIDE_LEFT], vecBox[SIDE_TOP]) + vecThicknessOffset, ImVec2(vecBox[SIDE_LEFT] + flCornerWidth, vecBox[SIDE_TOP]) + vecThicknessOffset },

			// top-right
			{ ImVec2(vecBox[SIDE_RIGHT] - flCornerWidth - vecThicknessOffset.x, vecBox[SIDE_TOP] + vecThicknessOffset.y * 2.0f), ImVec2(vecBox[SIDE_RIGHT] - vecThicknessOffset.x, vecBox[SIDE_TOP] + vecThicknessOffset.y * 2.0f), ImVec2(vecBox[SIDE_RIGHT] - vecThicknessOffset.x, vecBox[SIDE_TOP] + flCornerHeight + vecThicknessOffset.y * 2.0f) },

			// bottom-left
			{ ImVec2(vecBox[SIDE_LEFT] + flCornerWidth + vecThicknessOffset.x, vecBox[SIDE_BOTTOM] - vecThicknessOffset.y * 2.0f), ImVec2(vecBox[SIDE_LEFT] + vecThicknessOffset.x, vecBox[SIDE_BOTTOM] - vecThicknessOffset.y * 2.0f), ImVec2(vecBox[SIDE_LEFT] + vecThicknessOffset.x, vecBox[SIDE_BOTTOM] - flCornerHeight - vecThicknessOffset.y * 2.0f) },

			// bottom-right
			{ ImVec2(vecBox[SIDE_RIGHT], vecBox[SIDE_BOTTOM] - flCornerHeight) - vecThicknessOffset, ImVec2(vecBox[SIDE_RIGHT], vecBox[SIDE_BOTTOM]) - vecThicknessOffset, ImVec2(vecBox[SIDE_RIGHT] - flCornerWidth, vecBox[SIDE_BOTTOM]) - vecThicknessOffset }
		};

		for (std::size_t i = 0U; i < CS_ARRAYSIZE(arrCornerPoints); i++)
		{
			const auto& arrLinePoints = arrCornerPoints[i];
			const ImVec2 vecHalfPixelOffset = ((i & 1U) == 1U ? ImVec2(-0.5f, -0.5f) : ImVec2(0.5f, 0.5f));

			// @todo: we can even do not clear path and reuse it
			pDrawList->PathLineTo(arrLinePoints[0] + vecHalfPixelOffset);
			pDrawList->PathLineTo(arrLinePoints[1] + vecHalfPixelOffset);
			pDrawList->PathLineTo(arrLinePoints[2] + vecHalfPixelOffset);
			pDrawList->PathStroke(colOutline.GetU32(AlphaMultiplier), false, flThickness + 1.0f);

			pDrawList->PathLineTo(arrLinePoints[0] + vecHalfPixelOffset);
			pDrawList->PathLineTo(arrLinePoints[1] + vecHalfPixelOffset);
			pDrawList->PathLineTo(arrLinePoints[2] + vecHalfPixelOffset);
			pDrawList->PathStroke(colPrimary.GetU32(AlphaMultiplier), false, flThickness);
		}

		break;
	}
	default:
		break;
	}

	// accumulate spacing for next side/directional components
	for (float& flSidePadding : this->arrSidePaddings)
		flSidePadding += this->flComponentSpacing;

	return ImRect(vecBox).Contains(ImGui::GetIO().MousePos);
}

ImVec4 preview::Context_t::AddFrameComponent(ImDrawList* pDrawList, const ImVec2& vecScreen, const EAlignSide nSide, const Color_t& colBackground, const float flRounding)
{
	// calculate frame size by previously added components on active side
	const ImVec2 vecFrameSize = this->GetTotalDirectionalSize(nSide);

	ImVec2 vecFrameMin = { vecScreen.x - vecFrameSize.x * 0.5f, vecScreen.y - vecFrameSize.y };
	ImVec2 vecFrameMax = { vecScreen.x + vecFrameSize.x * 0.5f, vecScreen.y };

	pDrawList->AddRectFilled(vecFrameMin - this->flComponentSpacing, vecFrameMax + this->flComponentSpacing, colBackground.GetU32(), flRounding);

	// accumulate spacing for next side/directional components
	for (float& flSidePadding : this->arrSidePaddings)
		flSidePadding += this->flComponentSpacing;

	return { vecFrameMin.x, vecFrameMin.y, vecFrameMax.x, vecFrameMax.y };
}

/*
 * @todo: currently not well designed, make it more flexible for use cases where we need e.g. previous frame bar factor etc
 * also to optimize this, allocate components at stack instead of heap + make all context units static and do not realloc components storage every frame, but reset (like memset idk) it at the end of frame
 */
void preview::Context_t::AddComponent(CBaseComponent* pComponent)
{
	// guarantee that first directional component on each side is in the primary direction
	if (pComponent->IsDirectional())
	{
		CBaseDirectionalComponent* pDirectionalComponent = static_cast<CBaseDirectionalComponent*>(pComponent);

		// check if it's not an exception direction and there are no components in the primary direction
		if (((pDirectionalComponent->nSide & 1U) == 1U || pDirectionalComponent->nDirection != DIR_TOP) && this->arrSideDirectionPaddings[pDirectionalComponent->nSide][pDirectionalComponent->nSide] == 0.0f)
			pDirectionalComponent->nDirection = static_cast<EAlignDirection>(pDirectionalComponent->nSide);
	}

	float& flSidePadding = this->arrSidePaddings[pComponent->nSide];

	if (pComponent->IsDirectional())
	{
		CBaseDirectionalComponent* pDirectionalComponent = static_cast<CBaseDirectionalComponent*>(pComponent);
		float(&arrDirectionPaddings)[DIR_MAX] = this->arrSideDirectionPaddings[pDirectionalComponent->nSide];

		// directional components don't change side paddings, but take them into account
		pComponent->vecOffset[pDirectionalComponent->nSide & 1U] += ((pDirectionalComponent->nSide < 2U) ? -flSidePadding : flSidePadding);

		// check if the component is in the same direction as the side and it's the first component in this direction
		if (static_cast<std::uint8_t>(pDirectionalComponent->nDirection) == static_cast<std::uint8_t>(pDirectionalComponent->nSide) && arrDirectionPaddings[pDirectionalComponent->nDirection] == 0.0f)
		{
			// accumulate paddings for sub-directions
			for (std::uint8_t nSubDirection = DIR_LEFT; nSubDirection < DIR_MAX; nSubDirection++)
			{
				/*
				 * exclude conflicting sub-directions
				 *
				 * SIDE_LEFT[0]: DIR_LEFT[0], DIR_BOTTOM[3] | ~2 & ~1
				 * SIDE_TOP[1]: DIR_LEFT[0], DIR_TOP[1], DIR_RIGHT[2] | ~3
				 * SIDE_RIGHT[2]: DIR_RIGHT[2], DIR_BOTTOM[3] | ~0 & ~1
				 * SIDE_BOTTOM[3]: DIR_LEFT[0], DIR_RIGHT[2], DIR_BOTTOM[3] | ~1
				 */
				if (nSubDirection == pDirectionalComponent->nSide || nSubDirection == ((pDirectionalComponent->nSide + 2U) & 3U) || (nSubDirection == DIR_TOP && (pDirectionalComponent->nSide & 1U) == 0U))
					continue;

				arrDirectionPaddings[nSubDirection] += (pDirectionalComponent->vecSize[nSubDirection == DIR_BOTTOM ? SIDE_TOP : SIDE_LEFT] * (((pDirectionalComponent->nSide & 1U) == 1U) ? 0.5f : 1.0f) + this->flComponentSpacing);
			}
		}

		float& flSideDirectionPadding = arrDirectionPaddings[pDirectionalComponent->nDirection];

		// append direction padding to offset
		pComponent->vecOffset[pDirectionalComponent->nDirection & 1U] += ((pDirectionalComponent->nDirection < 2U) ? -flSideDirectionPadding : flSideDirectionPadding);

		// accumulate direction padding for next component
		flSideDirectionPadding += pDirectionalComponent->vecSize[pDirectionalComponent->nDirection & 1U];

		// accumulate spacing for next directional components
		flSideDirectionPadding += this->flComponentSpacing;
	}
	else
	{
		// append side padding to offset
		pComponent->vecOffset[pComponent->nSide & 1U] += ((pComponent->nSide < 2U) ? -(flSidePadding + pComponent->vecSize[pComponent->nSide]) : flSidePadding);

		// accumulate side padding for next component
		flSidePadding += pComponent->vecSize[pComponent->nSide & 1U];

		// accumulate spacing for next components
		flSidePadding += this->flComponentSpacing;
	}

	this->vecComponents.push_back(pComponent);
}

ImVec2 preview::Context_t::GetTotalDirectionalSize(const EAlignSide nSide) const
{
	ImVec2 vecSideSize = {};

	// @todo: we should peek max of bottom + side or top directions at horizontal sides
	const float(&arrDirectionPaddings)[DIR_MAX] = this->arrSideDirectionPaddings[nSide];
	for (std::uint8_t nSubDirection = DIR_LEFT; nSubDirection < DIR_MAX; nSubDirection++)
		vecSideSize[nSubDirection & 1U] += arrDirectionPaddings[nSubDirection];

	return vecSideSize;
}

void preview::Context_t::Render(ImDrawList* pDrawList, const ImVec4& vecBox) const
{
	bool bCenteredFirstSideDirectional[SIDE_MAX] = {};

	for (CBaseComponent* const pComponent : this->vecComponents)
	{
		ImVec2 vecPosition = pComponent->GetBasePosition(vecBox);

		// check if the component is in the side that supports multi-component centering
		if (pComponent->nSide == SIDE_TOP || pComponent->nSide == SIDE_BOTTOM)
		{
			// check if the component is directional
			if (CBaseDirectionalComponent* const pDirectionalComponent = static_cast<CBaseDirectionalComponent*>(pComponent); pDirectionalComponent->IsDirectional())
			{
				const float(&arrDirectionPaddings)[DIR_MAX] = this->arrSideDirectionPaddings[pComponent->nSide];

				// check if the component has horizontal direction
				if (static_cast<std::uint8_t>(pDirectionalComponent->nDirection) != static_cast<std::uint8_t>(pDirectionalComponent->nSide))
					// add centering offset to the component's offset
					pDirectionalComponent->vecOffset.x += (arrDirectionPaddings[DIR_LEFT] - arrDirectionPaddings[DIR_RIGHT]) * 0.5f;
				// otherwise check if it's the first component in direction as side
				else if (!bCenteredFirstSideDirectional[pDirectionalComponent->nSide])
				{
					// add centering offset to the component's offset
					pDirectionalComponent->vecOffset.x += (arrDirectionPaddings[DIR_LEFT] - arrDirectionPaddings[DIR_RIGHT]) * 0.5f;

					bCenteredFirstSideDirectional[pDirectionalComponent->nSide] = true;
				}
			}
		}

		// add final component offset to the base position
		vecPosition += pComponent->vecOffset;

		pComponent->Render(pDrawList, vecPosition);
	}
}

#pragma endregion
