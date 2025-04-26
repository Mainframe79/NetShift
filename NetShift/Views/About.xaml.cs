using System.Diagnostics;
using System.Windows;
using System.Windows.Input;


namespace NetShift.Views
{
    public partial class About : Window
    {
        public About()
        {
            InitializeComponent();
            ZentrixLogo.Source = new System.Windows.Media.Imaging.BitmapImage(new Uri("pack://application:,,,/Assets/ZentrixLabs_Logo.png"));

        }

        private void ZentrixLink_Click(object sender, MouseButtonEventArgs e)
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = "https://zentrixlabs.net",
                UseShellExecute = true
            });
        }
    }
}
