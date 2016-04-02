#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QDir>
#include <QDateTime>

class QLineEdit;
class QWidget;
class QVBoxLayout;
class QTableWidget;
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private slots:
	void on_browse_clicked();

private:
    void SetupGUI();

    QWidget* widget;
	QVBoxLayout* main_layout;


    QLineEdit* game_path;
	QSettings* settings;
	QTableWidget* table;

	void Information(QString s);

	QDir dir;

	void FindBuffs();

	void ParseLUA(const QString& file_path);

	void UpdateTable();

	const QString data_file_name = "OnyBuffCollecter.lua";
	
	class BuffData{
	public:
		BuffData(){}
		BuffData(const QString& n,QDateTime t) 
			: name(n),date_time(t){}

		QString name;
		QString guild;
		QDateTime date_time;
	};

	QMap<QString, BuffData> data_map;

	enum table_index{
		T_DATE,
		T_TIME,
		T_NAME,
		T_GUILD,
		T_NUM_COLS
	};
};

#endif // MAINWINDOW_H
