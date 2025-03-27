using System.IO;
using System.Windows;
using NetShift.ViewModels;

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
    }
}