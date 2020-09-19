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
#include "TempoDialog.h"
#include "CommandIDs.h"
#include "ColourIDs.h"
#include "HelioTheme.h"

class TapTempoComponent final : public Component, private Timer
{
public:

    TapTempoComponent() :
        targetColour(Colours::white.withAlpha(0.01f)),
        highlightColour(Colours::white.withAlpha(0.05f))
    {
        this->currentFillColour = this->targetColour;
    }

    Function<void(int newTempoBpm)> onTempoChanged;

    void paint(Graphics &g) override
    {
        // todo frame

        g.setColour(this->currentFillColour);
        g.fillRect(this->getLocalBounds());

        HelioTheme::drawNoiseWithin(this->getLocalBounds().reduced(1), g, 10.f);
    }

    void mouseDown(const MouseEvent &e) override
    {
        this->detectAndSendTapTempo();
        this->currentFillColour = this->highlightColour; // then animate
        this->startTimerHz(60);
    }

private:

    void detectAndSendTapTempo()
    {
        // todo
        const auto now = Time::getMillisecondCounterHiRes();


        if (this->onTempoChanged != nullptr)
        {
            // todo
        }
    }

    void timerCallback() override
    {
        const auto newColour = this->currentFillColour.
            interpolatedWith(this->targetColour, 0.1f);

        if (this->currentFillColour == newColour)
        {
            this->stopTimer();
        }
        else
        {
            this->currentFillColour = newColour;
            this->repaint();
        }
    }

    Colour currentFillColour;
    const Colour targetColour;
    const Colour highlightColour;

    double lastTapMs = 0.0;
    Array<double> tapIntervalsMs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TapTempoComponent)
};

TempoDialog::TempoDialog(int bpmValue) :
    originalValue(bpmValue),
    newValue(bpmValue)
{
    this->messageLabel = make<Label>();
    this->addAndMakeVisible(this->messageLabel.get());
    this->messageLabel->setFont({ 21.f });
    this->messageLabel->setJustificationType(Justification::centred);

    this->cancelButton = make<TextButton>();
    this->addAndMakeVisible(this->cancelButton.get());
    this->cancelButton->onClick = [this]()
    {
        this->doCancel();
    };

    this->okButton = make<TextButton>();
    this->addAndMakeVisible(this->okButton.get());
    this->okButton->onClick = [this]()
    {
        this->doOk();
    };

    this->tapTempo = make<TapTempoComponent>();
    this->addAndMakeVisible(this->tapTempo.get());
    this->tapTempo->onTempoChanged = [this](int newTempoBpm)
    {
        this->newValue = newTempoBpm;
        // update text field
    };

    this->textEditor = make<TextEditor>();
    this->addAndMakeVisible(this->textEditor.get());
    this->textEditor->setMultiLine(false);
    this->textEditor->setReturnKeyStartsNewLine(false);
    this->textEditor->setReadOnly(false);
    this->textEditor->setScrollbarsShown(true);
    this->textEditor->setCaretVisible(true);
    this->textEditor->setPopupMenuEnabled(true);
    this->textEditor->setFont({ 21.f });

    this->textEditor->onTextChange = [this]()
    {
        this->updateOkButtonState();
        this->newValue = this->textEditor->getText().getIntValue();
        // reset tap-tempo? or is that not necessary?
    };

    this->textEditor->onFocusLost = [this]()
    {
        this->onTextFocusLost();
    };

    this->textEditor->onReturnKey = [this]()
    {
        this->onTextFocusLost();
    };

    this->textEditor->onEscapeKey = [this]()
    {
        this->doCancel();
    };

    this->textEditor->setText(String(this->originalValue), dontSendNotification);

    this->messageLabel->setText(TRANS(I18n::Dialog::setTempoCaption), dontSendNotification);
    this->messageLabel->setInterceptsMouseClicks(false, false);

    this->okButton->setButtonText(TRANS(I18n::Dialog::apply));
    this->cancelButton->setButtonText(TRANS(I18n::Dialog::cancel));

    this->setSize(450, 250);
    this->updatePosition();
    this->updateOkButtonState();
}

// still need to have this empty dtor here,
// so that compiler can resolve the forward-declared TapTempo
TempoDialog::~TempoDialog() {}

void TempoDialog::resized()
{
    this->messageLabel->setBounds(this->getCaptionBounds());

    const auto buttonsBounds(this->getButtonsBounds());
    const auto buttonWidth = buttonsBounds.getWidth() / 2;

    this->okButton->setBounds(buttonsBounds.withTrimmedLeft(buttonWidth));
    this->cancelButton->setBounds(buttonsBounds.withTrimmedRight(buttonWidth + 1));

    this->textEditor->setBounds(this->getRowBounds(0.3f, DialogBase::textEditorHeight));
    this->tapTempo->setBounds(this->getRowBounds(0.7f,
        TempoDialog::tapTempoHeight, TempoDialog::tapTempoMargin));
}

void TempoDialog::parentHierarchyChanged()
{
    this->updatePosition();
}

void TempoDialog::parentSizeChanged()
{
    this->updatePosition();
}

void TempoDialog::handleCommandMessage(int commandId)
{
    if (commandId == CommandIDs::DismissModalDialogAsync)
    {
        this->doCancel();
    }
}

void TempoDialog::inputAttemptWhenModal()
{
    this->postCommandMessage(CommandIDs::DismissModalDialogAsync);
}

void TempoDialog::updateOkButtonState()
{
    const bool textIsEmpty = this->textEditor->getText().isEmpty();
    this->okButton->setAlpha(textIsEmpty ? 0.5f : 1.f);
    this->okButton->setEnabled(!textIsEmpty);
    // todo check range?
}

void TempoDialog::onTextFocusLost()
{
    this->updateOkButtonState();

    const auto *focusedComponent = Component::getCurrentlyFocusedComponent();
    if (this->textEditor->getText().isNotEmpty() &&
        focusedComponent != this->okButton.get() &&
        focusedComponent != this->cancelButton.get())
    {
        this->doOk();
    }
    else
    {
        this->textEditor->grabKeyboardFocus();
    }
}

void TempoDialog::doCancel()
{
    if (this->onCancel != nullptr)
    {
        BailOutChecker checker(this);

        this->onCancel();

        if (checker.shouldBailOut())
        {
            jassertfalse; // not expected
            return;
        }
    }

    this->dismiss();
}

void TempoDialog::doOk()
{
    if (this->textEditor->getText().isNotEmpty())
    {
        if (this->onOk != nullptr)
        {
            BailOutChecker checker(this);

            this->onOk(this->newValue);

            if (checker.shouldBailOut())
            {
                jassertfalse; // not expected
                return;
            }
        }

        this->dismiss();
    }
}