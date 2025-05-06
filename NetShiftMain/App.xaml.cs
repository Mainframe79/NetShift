using System.Net;
using System.Windows;

namespace NetShift
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            // Allow TLS 1.2 and 1.3 (if supported by OS)
            ServicePointManager.SecurityProtocol =
                SecurityProtocolType.Tls12 | SecurityProtocolType.Tls13;

            base.OnStartup(e);
        }
    }

}
