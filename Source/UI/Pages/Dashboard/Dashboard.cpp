/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Common.h"
#include "Dashboard.h"
#include "SpectralLogo.h"
#include "UserProfileComponent.h"
#include "LoginButton.h"
#include "OverlayButton.h"
#include "DashboardMenu.h"
#include "PanelBackgroundA.h"
#include "PanelBackgroundB.h"
#include "OpenProjectButton.h"
#include "CreateProjectButton.h"
#include "UpdatesInfoComponent.h"
#include "SeparatorVerticalSkew.h"
#include "SeparatorHorizontalFadingReversed.h"

Dashboard::Dashboard(Workspace &workspace) : workspace(workspace)
{
    this->setFocusContainer(false);
    this->setWantsKeyboardFocus(false);
    this->setPaintingIsUnclipped(true);

    this->backgroundA = make<PanelBackgroundA>();
    this->addAndMakeVisible(this->backgroundA.get());

    this->patreonLabel = make<Label>();
    this->addAndMakeVisible(this->patreonLabel.get());

    this->patreonLabel->setFont(Globals::UI::Fonts::S);
    this->patreonLabel->setJustificationType(Justification::centred);

    this->userProfile = make<UserProfileComponent>();
    this->addAndMakeVisible(this->userProfile.get());

    this->loginButton = make<LoginButton>();
    this->addAndMakeVisible(this->loginButton.get());

    this->backgroundB = make<PanelBackgroundB>();
    this->addAndMakeVisible(this->backgroundB.get());

    this->openProjectButton = make<OpenProjectButton>();
    this->addAndMakeVisible(this->openProjectButton.get());

    this->createProjectButton = make<CreateProjectButton>();
    this->addAndMakeVisible(this->createProjectButton.get());

    this->createProjectCombo = make<MobileComboBox::Container>();
    this->addAndMakeVisible(this->createProjectCombo.get());

    this->skew = make<SeparatorVerticalSkew>();
    this->addAndMakeVisible(this->skew.get());
    this->logo = make<SpectralLogo>();
    this->addAndMakeVisible(this->logo.get());

    this->projectsList = make<DashboardMenu>(this->workspace);
    this->addAndMakeVisible(this->projectsList.get());

    this->separator = make<SeparatorHorizontalFadingReversed>();
    this->addAndMakeVisible(this->separator.get());

    this->updatesInfo = make<UpdatesInfoComponent>();
    this->addAndMakeVisible(this->updatesInfo.get());

    this->patreonButton = make<OverlayButton>();
    this->addAndMakeVisible(this->patreonButton.get());

    this->patreonLabel->setText(TRANS(I18n::Common::supportProject), dontSendNotification);
    this->patreonLabel->setColour(Label::textColourId,
        findDefaultColour(Label::textColourId).withMultipliedAlpha(0.25f));
    this->patreonButton->onClick = []()
    {
        URL url("https://www.patreon.com/peterrudenko");
        url.launchInDefaultBrowser();
    };

    this->updateProfileViews();

    this->workspace.getUserProfile().addChangeListener(this);
}

Dashboard::~Dashboard()
{
    this->workspace.getUserProfile().removeChangeListener(this);
}

void Dashboard::resized()
{
    // background stuff:
    constexpr auto skewWidth = 64;
    constexpr auto leftSectionWidth = 320;
    constexpr auto rightSectionX = leftSectionWidth + skewWidth;
    const auto rightSectionWidth = this->getWidth() - rightSectionX;

    this->backgroundA->setBounds(0, 0, leftSectionWidth, this->getHeight());
    this->skew->setBounds(leftSectionWidth, 0, skewWidth, this->getHeight());
    this->backgroundB->setBounds(rightSectionX, 0, rightSectionWidth, this->getHeight());

    // left section content:
    constexpr auto logoSize = 280;
    constexpr auto logoX = leftSectionWidth / 2 - logoSize / 2;
    this->logo->setBounds(logoX, 32, logoSize, logoSize);

    constexpr auto buttonWidth = 264;
    constexpr auto buttonHeight = 32;

    constexpr auto helperX = leftSectionWidth / 2 - buttonWidth / 2;
    this->updatesInfo->setBounds(helperX, 352, buttonWidth, 172);
    this->userProfile->setBounds(helperX, getHeight() - 110, buttonWidth, 56);
    this->loginButton->setBounds(helperX, getHeight() - 110, buttonWidth, 56);

    this->separator->setBounds(helperX, getHeight() - 50, buttonWidth, 3);
    this->patreonLabel->setBounds(helperX, getHeight() - 44, buttonWidth, buttonHeight);
    this->patreonButton->setBounds(helperX, getHeight() - 44, buttonWidth, buttonHeight);

    // right section content:
    constexpr auto projectsListWidth = 400;
    constexpr auto buttonsX = rightSectionX + 16;
    this->openProjectButton->setBounds(buttonsX, 16, buttonWidth, buttonHeight);
    this->createProjectButton->setBounds(buttonsX, 52, buttonWidth, buttonHeight);
    this->createProjectCombo->setBounds(buttonsX, 52, buttonWidth, 140);

    this->projectsList->setBounds(this->getWidth() - projectsListWidth,
        10, projectsListWidth, this->getHeight() - 20);
}

void Dashboard::changeListenerCallback(ChangeBroadcaster *source)
{
    // Listens to user profile changes:
    this->updateProfileViews();
}

void Dashboard::updateProfileViews()
{
    const bool loggedIn = this->workspace.getUserProfile().isLoggedIn();
    this->loginButton->setVisible(!loggedIn);
    this->userProfile->setVisible(loggedIn);
    if (loggedIn)
    {
        this->userProfile->updateProfileInfo();
    }
    this->projectsList->updateListContent();
}
