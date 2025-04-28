using System.IO;
using System.Windows;
using System.Windows.Input;
using NetShift.ViewModels;
using NetShift.Views;

namespace NetShift
{
    public partial class MainWindow : Window
    {
        private readonly string _firstLaunchFlagPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "NetShift", "firstlaunch.flag");

        public MainWindow()
        {
            InitializeComponent();
            DataContext = new NetShift.ViewModels.MainViewModel();

        }

        private void AboutLink_Click(object sender, MouseButtonEventArgs e)
        {
            var about = new About();
            about.Owner = this;
            about.ShowDialog();
        }

    }
}