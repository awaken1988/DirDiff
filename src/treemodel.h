#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <functional>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QThread>
#include "fsdiff.h"



class TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    TreeModel(QObject *parent, std::shared_ptr<fsdiff::diff_t> aDiffTree);
    ~TreeModel();

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;


	void startFileHash( std::function<void()> aOnReady, std::function<void(int,int,int)> aStep  );

signals:
	void operateFilehash();

private:
    void setupModelData();

//TODO: make this private again
public:
    shared_ptr<fsdiff::diff_t>  rootItem;

};

class FilehashWorker : public QObject
{
	Q_OBJECT

public:
	FilehashWorker(shared_ptr<fsdiff::diff_t>& aTree);
	virtual ~FilehashWorker();
public slots:
	void hashAllFiles();
signals:
	void resultReady();
	void stepReady(int,int,int);

private:
	shared_ptr<fsdiff::diff_t>& m_tree;
};



#endif // TREEMODEL_H
