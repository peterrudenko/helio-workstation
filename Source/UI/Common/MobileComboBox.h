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

#pragma once

#include "IconButton.h"
#include "MenuPanel.h"
#include "ShadowDownwards.h"
#include "SeparatorHorizontalReversed.h"

class MobileComboBox final : public Component
{
public:

    MobileComboBox(WeakReference<Component> editor, WeakReference<Component> primer);
    ~MobileComboBox();

    void initMenu(MenuPanel::Menu menu);

    class Trigger final : public IconButton
    {
    public:

        Trigger(WeakReference<Component> listener = nullptr);
        void parentHierarchyChanged() override;
        void parentSizeChanged() override;
        void updateBounds();

    private:

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Trigger)
    };

    class Primer final : public Component
    {
    public:

        Primer();
        ~Primer() override;

        void handleCommandMessage(int commandId) override;

        void initWith(WeakReference<Component> textEditor,
            MenuPanel::Menu menu,
            Component *newCustomBackground = nullptr);

        void initWith(WeakReference<Component> textEditor,
            Function<MenuPanel::Menu(void)> menuInitializer,
            Component *newCustomBackground = nullptr);

        void updateMenu(MenuPanel::Menu menu);

        // this needs to be called before target text editor is deleted
        void cleanup();

    private:

        ComponentAnimator animator;
        UniquePointer<MobileComboBox> combo;
        UniquePointer<MobileComboBox::Trigger> comboTrigger;
        WeakReference<Component> textEditor;
        Function<MenuPanel::Menu(void)> menuInitializer;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Primer)
    };

    void resized() override;
    void parentHierarchyChanged() override;
    void parentSizeChanged() override;
    void handleCommandMessage(int commandId) override;

private:

    void initCaption(TextEditor *editor);
    void initCaption(Label *label);
    void initCaption(const String &text);

    void initBackground(Component *newCustomBackground);

    WeakReference<Component> primer;
    WeakReference<Component> editor;
    ComponentAnimator animator;

    bool hasCaption = true;
    UniquePointer<Component> background;
    UniquePointer<MenuPanel> menu;
    UniquePointer<MobileComboBox::Trigger> triggerButtton;
    UniquePointer<ShadowDownwards> shadow;
    UniquePointer<SeparatorHorizontalReversed> separator;
    UniquePointer<Label> currentNameLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MobileComboBox)
};
