using System.Windows;
using System.Windows.Input;

namespace NetShift.ViewModels
{
    public class PresetNameDialogViewModel : NotifyPropertyChangedBase
    {
        private string? _presetName;
        public string? PresetName
        {
            get => _presetName;
            set { _presetName = value; OnPropertyChanged(); }
        }

        public ICommand OkCommand { get; }

        public PresetNameDialogViewModel()
        {
            OkCommand = new RelayCommand(parameter =>
            {
                var window = Application.Current.Windows.OfType<NetShift.Views.PresetNameDialog>().FirstOrDefault();
                if (window != null)
                {
                    window.DialogResult = true;
                    window.Close();
                }
            });
        }
    }
}