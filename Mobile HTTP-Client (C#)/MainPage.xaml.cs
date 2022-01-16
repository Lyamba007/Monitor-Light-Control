using System;
using System.Net.Http;
using Xamarin.Forms;
using System.Net;
using System.Threading;

namespace MonitorLight
{
	public partial class MainPage : ContentPage
	{
        private byte isEnabled; //Mobile app enable variable

		//Set the RGB palette with OFF mode
		//
        const byte RED         = 255;
        const byte GREEN       = 255;
        const byte BLUE        = 255;
        const byte EMPTY_COLOR = 0;		//LED strip is off

        public MainPage()
        {
            InitializeComponent();
            BlockApp();	//First app launch should not interrupt the control device work without button tapping
        }

		//Start menu before control transfer to Mobile app 
		//
        private void BlockApp()
        {
            isEnabled = 0;

            StopButton.IsVisible       = false;
            ColorChoiceLabel.IsVisible = false;
            ColorsGrid.IsVisible       = false;
            StartButton.IsVisible      = true;

            Reminder.Text = "Push for start";
        }

		//Mobile app control menu
		//
        private void UnblockApp()
        {
            isEnabled = 1;

            StartButton.IsVisible      = false;
            StopButton.IsVisible       = true;
            ColorChoiceLabel.IsVisible = true;
            ColorsGrid.IsVisible       = true;

            Reminder.Text = "Push for stop";
        }

		//Request (Event) handler
		//
        private async void SendValues(byte isEnabled, byte r, byte g, byte b)
        {
            string request = String.Format(
            "http://192.168.1.1/?isEnabled={0}&r={1}&g={2}&b={3}", isEnabled,r,g,b); //Create the request by rule (IP and arguments)	

            var uri = new Uri(request); 											 //Create the Uniform Resource Identifier

            HttpClient Client = new HttpClient();                                    //Create the HTTP instance

            while (true)
            {
                var response = await Client.PostAsync(uri, null);                    //Asynchronous polling the server 

                if (response.StatusCode == HttpStatusCode.OK)                        //If connection has been established (200 code has been sent)
                { 
                    break;
                }

                Thread.Sleep(500);                                                   //Polling server each 500 ms
            }
        }

		//If the main button is clicked than the isEnabled value (1) and default color values (0) should be sent to control device 
		//
        private void StartButton_Clicked(object sender, EventArgs e)
        {
            UnblockApp();
            SendValues(isEnabled, EMPTY_COLOR, EMPTY_COLOR, EMPTY_COLOR);
        }

		
		//The Mobile app has stopped. The start menu has been reset and isEnabled value is 0 as the LEDs color palette
		//
        private void StopButton_Clicked(object sender, EventArgs e)
        {
            BlockApp();
            SendValues(isEnabled, EMPTY_COLOR, EMPTY_COLOR, EMPTY_COLOR);
        }

		//Set the LED strip to red (1, 255, 0, 0) 
		//isEnabled value is always "1"
		//
        private void RedButton_Clicked(object sender, EventArgs e)
        {
            SendValues(isEnabled, RED, EMPTY_COLOR, EMPTY_COLOR);
        }

		//Set the LED strip to red (1, 255, 255, 0) 
		//
        private void YellowButton_Clicked(object sender, EventArgs e)
        {
            SendValues(isEnabled, RED, GREEN, EMPTY_COLOR);
        }

		//Set the LED strip to green (1, 0, 255, 0)   
		//
        private void GreenButton_Clicked(object sender, EventArgs e)
        {
            SendValues(isEnabled, EMPTY_COLOR, GREEN, EMPTY_COLOR);
        }

	    //Set the LED strip to blue (1, 0, 0, 255) 
		//
        private void BlueButton_Clicked(object sender, EventArgs e)
        {
            SendValues(isEnabled, EMPTY_COLOR, EMPTY_COLOR, BLUE);
        }

    }    
}
