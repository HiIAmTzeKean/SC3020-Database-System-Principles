#ifndef RECORD_H
#define RECORD_H

struct Record {
    char game_date_est[11];
    int team_id_home;
    int pts_home;
    float fg_pct_home;
    float ft_pct_home;
    float fg3_pct_home;
    int ast_home;
    int reb_home;
    bool home_team_wins;

    static int size() {
        return sizeof(Record);
    }
};

#endif // RECORD_H