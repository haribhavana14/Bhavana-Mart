#include <iostream>
#include <fstream>
#include <string>

using namespace std;

// Function for registration
void registerUser()
{
    string email, password;

    cout << "\n===== REGISTER =====\n";

    cout << "Enter Email ID: ";
    cin >> email;

    cout << "Enter Password: ";
    cin >> password;

    // Check whether email already exists
    ifstream checkFile("users.txt");

    string storedEmail, storedPassword;
    while (checkFile >> storedEmail >> storedPassword)
    {
        if (storedEmail == email)
        {
            cout << "\nEmail already registered!\n";
            checkFile.close();
            return;
        }
    }

    checkFile.close();

    // Open file in append mode
    // Existing users will NOT be deleted
    ofstream file("users.txt", ios::app);

    if (file.is_open())
    {
        file << email << " " << password << endl;
        file.close();

        cout << "\nRegistration successful!\n";
    }
    else
    {
        cout << "\nError opening file!\n";
    }
}

// Function for login
void loginUser()
{
    string email, password;
    string storedEmail, storedPassword;

    cout << "\n===== LOGIN =====\n";

    cout << "Enter Email ID: ";
    cin >> email;

    cout << "Enter Password: ";
    cin >> password;

    ifstream file("users.txt");

    bool loginSuccess = false;

    while (file >> storedEmail >> storedPassword)
    {
        if (storedEmail == email && storedPassword == password)
        {
            loginSuccess = true;
            break;
        }
    }

    file.close();

    if (loginSuccess)
    {
        cout << "\nLogin Successful! Welcome!\n";
    }
    else
    {
        cout << "\nInvalid Email ID or Password!\n";
    }
}

// Main function
int main()
{
    int choice;

    do
    {
        cout << "\n========================\n";
        cout << "      LOGIN SYSTEM\n";
        cout << "========================\n";
        cout << "1. Register\n";
        cout << "2. Login\n";
        cout << "3. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice)
        {
            case 1:
                registerUser();
                break;

            case 2:
                loginUser();
                break;

            case 3:
                cout << "\nThank you!\n";
                break;

            default:
                cout << "\nInvalid choice! Try again.\n";
        }

    } while (choice != 3);

    return 0;
}