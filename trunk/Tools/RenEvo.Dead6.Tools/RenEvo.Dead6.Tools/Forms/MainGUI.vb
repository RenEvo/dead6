'Documentation: Logging.LogTraceListener

Public Class MainGUI

    Private Sub MainGUI_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Trace.WriteLine("Application Startup")
    End Sub

    Private Sub AboutToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles AboutToolStripMenuItem.Click
        MessageBox.Show("© 2007 RenEvo Software & Designs", "About", MessageBoxButtons.OK, MessageBoxIcon.Information, MessageBoxDefaultButton.Button1)
    End Sub

    Private Sub ExitToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ExitToolStripMenuItem.Click
        Application.Exit()
    End Sub

    Private Sub LogFileToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles LogFileToolStripMenuItem.Click
        Process.Start(String.Format("{0}\notepad.exe", Environment.GetFolderPath(Environment.SpecialFolder.System)), My.Application.Log.DefaultFileLogWriter.FullLogFileName)
    End Sub

End Class
