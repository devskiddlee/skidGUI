#include <skidGUI.h>

void ExampleButtonEvent(Button* button, ButtonEvent e) {
    // Only print if Event was a Click Event
    if (e.type == OnClick) {
        std::cout << "Button " << button->id << " pressed!" << std::endl;
    }
}

static int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    //Create a template 'Press Me!' Button
    Button button;
    button.label = L"Press Me!"; // the text which will appear on the button
    button.color = Color(255, 200, 200, 200); // Its background color
    button.fontColor = Color(255, 0, 0, 0);
    button.fontName = L"Arial";
    button.fontSize = 24;
    button.x = 10;
    button.y = 10; 
    button.width = 200;
    button.height = 50;

    button.borderSize = 2;
    button.borderColor = Color(255, 30, 30, 30);

    button.clbk = ExampleButtonEvent; // Event Callback

    // Add this button 3 times inside layer group 1 (default is 0)
    UI::AddWidget<Button>(button, 1);
    UI::AddWidget<Button>(button, 1);
    UI::AddWidget<Button>(button, 1);

    // Set the Layout Type of layer group 1 to Horizontal Fill
    UI::SetLayoutType(1, HorizontalFill);

    // Shows the Debug Console
    UI::EnableDebugConsole();

    // Sets Background color of the Window and Initializes it
    UI::backgroundColor = Color(255, 255, 255, 255);
    UI::Init(hInst, L"skidGUI Example", 800, 600);

    // Runs the main loop
    UI::Run();

    return 0;
}