CREATE TABLE title_akas (
    titleId TEXT,
    ordering INT,
    title TEXT,
    region TEXT,
    language TEXT,
    types TEXT,
    attributes TEXT,
    isOriginalTitle BOOLEAN,
    PRIMARY KEY (titleId, ordering)
);
