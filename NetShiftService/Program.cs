using System.ServiceProcess;

namespace NetShiftService
{
    static class Program
    {
        static void Main()
        {
            ServiceBase[] ServicesToRun;
            ServicesToRun = new ServiceBase[]
            {
                new NetShiftService()
            };
            ServiceBase.Run(ServicesToRun);
        }
    }
}